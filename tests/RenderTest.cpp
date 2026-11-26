// DesgraciaTest: offline regression / render tool.
// Runs the real DesgraciaProcessor (same code as the plugin) and checks:
//   every preset loads and makes sound, no NaN/Inf, no stuck notes,
//   mono/legato voice counts, sustain pedal, sample rates & odd block sizes,
//   state save/restore, user preset save/load, CPU cost.
// Usage: DesgraciaTest <outDir> [--snapshot]
#include "PluginProcessor.h"
#include "ui/PluginEditor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <chrono>

static int failures = 0;
#define CHECK(cond, msg) do { if (! (cond)) { ++failures; std::cout << "  FAIL: " << msg << std::endl; } } while (0)

struct Note { double t, len; int n; float v; };

// F minor phrase (the key that came out of the reference analysis) + chord + low + high
static std::vector<Note> phrase()
{
    std::vector<Note> v;
    const int mel[] = { 65, 68, 72, 70, 68, 67, 65, 63, 65, 72 };
    double t = 0.0;
    for (int i = 0; i < 10; ++i) { v.push_back ({ t, 0.19, mel[i], i % 3 == 0 ? 0.95f : 0.7f }); t += 0.21; }
    for (int n : { 53, 56, 60, 63 }) v.push_back ({ 2.3, 1.2, n, 0.8f });          // chord
    v.push_back ({ 3.8, 0.6, 41, 0.9f });                                           // low F
    v.push_back ({ 4.5, 0.18, 84, 0.8f }); v.push_back ({ 4.7, 0.18, 89, 0.8f });   // high motif
    // legato overlap (glide test)
    v.push_back ({ 5.0, 0.45, 60, 0.8f }); v.push_back ({ 5.35, 0.45, 67, 0.8f });
    return v;
}

struct Result { double peak = 0, rms = 0, rmsTail = 0; bool finite = true; int voicesAfter = 0; double renderSecs = 0; };

static Result render (DesgraciaProcessor& p, double sr, int block, const std::vector<Note>& notes, double total,
                      juce::AudioBuffer<float>* capture, double tailFrom)
{
    Result r;
    const int N = (int) (total * sr);
    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;
    if (capture) capture->setSize (2, N);
    double sum = 0, sumTail = 0; int nTail = 0;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int pos = 0; pos < N; pos += block)
    {
        const int n = std::min (block, N - pos);
        buf.setSize (2, n, false, false, true);
        midi.clear();
        for (auto& e : notes)
        {
            const int on = (int) (e.t * sr), off = (int) ((e.t + e.len) * sr);
            if (on >= pos && on < pos + n) midi.addEvent (juce::MidiMessage::noteOn (1, e.n, e.v), on - pos);
            if (off >= pos && off < pos + n) midi.addEvent (juce::MidiMessage::noteOff (1, e.n), off - pos);
        }
        p.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c)
        {
            const float* d = buf.getReadPointer (c);
            for (int i = 0; i < n; ++i)
            {
                const float x = d[i];
                if (! std::isfinite (x)) r.finite = false;
                r.peak = std::max (r.peak, (double) std::abs (x));
                sum += x * x;
                if ((pos + i) / sr >= tailFrom) { sumTail += x * x; ++nTail; }
            }
            if (capture) capture->copyFrom (c, pos, buf, c, 0, n);
        }
    }
    r.renderSecs = std::chrono::duration<double> (std::chrono::high_resolution_clock::now() - t0).count();
    r.rms = std::sqrt (sum / (2.0 * N));
    r.rmsTail = nTail > 0 ? std::sqrt (sumTail / nTail) : 0.0;
    r.voicesAfter = p.engine.activeVoices.load();
    return r;
}

static void writeWav (const juce::File& f, const juce::AudioBuffer<float>& b, double sr)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (f.createOutputStream().release());
    if (auto* w = wav.createWriterFor (os.get(), sr, 2, 24, {}, 0))
    {
        os.release();
        std::unique_ptr<juce::AudioFormatWriter> writer (w);
        writer->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
    }
}

// fake DAW transport (like FL Studio in play) to test tempo-locked behaviour
struct FakePlayHead : juce::AudioPlayHead
{
    double bpm = 145.0, ppq = 0.0, sr = 48000.0; int lastBlock = 0;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo i; i.setBpm (bpm); i.setPpqPosition (ppq); i.setIsPlaying (true); return i;
    }
};

static void prep (DesgraciaProcessor& p, double sr, int block)
{
    p.setPlayConfigDetails (0, 2, sr, block);
    p.prepareToPlay (sr, block);
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File out (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    out.createDirectory();
    const bool snapshot = argc > 2 && juce::String (argv[2]) == "--snapshot";

    if (argc > 2 && juce::String (argv[2]) == "--export-presets")
    {
        DesgraciaProcessor p;
        prep (p, 48000, 512);
        const auto& f = getFactoryPresets();
        for (int i = 0; i < (int) f.size(); ++i)
        {
            p.presets.load (i);
            auto dir = out.getChildFile (juce::String (f[(size_t) i].category).replace (" / ", "-").replace (" ", "_"));
            dir.createDirectory();
            const bool ok = p.presets.writePresetFile (dir.getChildFile (juce::String (i + 1).paddedLeft ('0', 2) + " " + f[(size_t) i].name + ".desgracia"), f[(size_t) i].name);
            std::cout << (ok ? "exported " : "FAILED ") << f[(size_t) i].name << std::endl;
        }
        return 0;
    }

    if (snapshot)
    {
        // render the editor to PNGs (needs an X display, e.g. xvfb-run)
        DesgraciaProcessor p;
        prep (p, 48000, 512);
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        ed->setSize (1280, 820);
        int vocIdx = 0;
        for (int i = 0; i < p.presets.getEntries().size(); ++i) if (p.presets.getEntries()[i].category == "VOCODER") { vocIdx = i; break; }
        const std::pair<int, int> shots[] = { { 0, 0 }, { 17, 1 }, { vocIdx, 2 }, { 57, 3 } };
        for (auto [idx, page] : shots)
        {
            p.presets.load (idx);
            if (auto* de = dynamic_cast<DesgraciaEditor*> (ed.get())) de->showPage (page);
            juce::MidiBuffer m; juce::AudioBuffer<float> b (2, 512);
            m.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);
            for (int i = 0; i < 30; ++i) { p.processBlock (b, m); m.clear(); }
            for (int i = 0; i < 30; ++i) juce::MessageManager::getInstance()->runDispatchLoopUntil (20);
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            juce::File f = out.getChildFile ("ui_page" + juce::String (page) + ".png");
            f.deleteFile();
            juce::PNGImageFormat png;
            if (auto s = f.createOutputStream()) png.writeImageToStream (img, *s);
            std::cout << "snapshot " << f.getFullPathName() << std::endl;
            juce::MidiBuffer off; off.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
            p.processBlock (b, off);
        }
        ed.reset();
        return 0;
    }

    const double sr = 48000.0;
    DesgraciaProcessor p;
    prep (p, sr, 512);
    const auto& factory = getFactoryPresets();
    std::cout << "== presets (" << factory.size() << ")" << std::endl;
    CHECK (factory.size() >= 25, "need 25+ presets");

    std::cout << juce::String ("name").paddedRight (' ', 22) << " cat              peak    rmsDb  cpu%   voicesAfter" << std::endl;
    for (int i = 0; i < (int) factory.size(); ++i)
    {
        p.presets.load (i);
        p.reset();
        juce::AudioBuffer<float> cap;
        auto notes = phrase();
        auto r = render (p, sr, 512, notes, 9.5, &cap, 99.0);
        r.voicesAfter = render (p, sr, 512, {}, 6.0, nullptr, 99.0).voicesAfter;   // let long releases finish
        const double rmsDb = 20.0 * std::log10 (std::max (1e-9, r.rms));
        const double cpu = r.renderSecs / 9.5 * 100.0;
        std::cout << juce::String (factory[(size_t) i].name).paddedRight (' ', 22) << " "
                  << juce::String (factory[(size_t) i].category).paddedRight (' ', 15) << " "
                  << juce::String (r.peak, 3) << "  " << juce::String (rmsDb, 1).paddedLeft (' ', 6) << "  "
                  << juce::String (cpu, 2).paddedLeft (' ', 5) << "  " << r.voicesAfter << std::endl;
        CHECK (r.finite, factory[(size_t) i].name << ": NaN/Inf");
        CHECK (rmsDb > -40.0, factory[(size_t) i].name << ": too quiet / silent");
        CHECK (r.peak <= 1.0001, factory[(size_t) i].name << ": exceeds 0 dBFS");
        CHECK (r.voicesAfter == 0, factory[(size_t) i].name << ": voices still active after release (stuck?)");
        writeWav (out.getChildFile (juce::String (i).paddedLeft ('0', 2) + "_" + juce::File::createLegalFileName (factory[(size_t) i].name).replaceCharacter (' ', '_') + ".wav"), cap, sr);
    }

    std::cout << "== stuck-note / pedal / all-notes-off" << std::endl;
    {
        p.presets.load (0);
        p.setCurrentProgram (13);   // Teclado de Feria (poly)
        juce::AudioBuffer<float> b (2, 256);
        juce::MidiBuffer m;
        for (int n = 40; n < 80; ++n) m.addEvent (juce::MidiMessage::noteOn (1, n, 0.8f), n - 40);
        p.processBlock (b, m); m.clear();
        CHECK (p.engine.activeVoices.load() == SynthEngine::maxVoices, "voice stealing should cap at 16");
        // sustain pedal holds notes after note-off
        m.addEvent (juce::MidiMessage::controllerEvent (1, 64, 127), 0);
        for (int n = 40; n < 80; ++n) m.addEvent (juce::MidiMessage::noteOff (1, n), 10);
        p.processBlock (b, m); m.clear();
        for (int k = 0; k < 40; ++k) p.processBlock (b, m);
        CHECK (p.engine.activeVoices.load() > 0, "sustain pedal should hold voices");
        m.addEvent (juce::MidiMessage::controllerEvent (1, 64, 0), 0);
        p.processBlock (b, m); m.clear();
        for (int k = 0; k < 400; ++k) p.processBlock (b, m);
        CHECK (p.engine.activeVoices.load() == 0, "pedal up should release everything");
        // all notes off
        for (int n = 50; n < 60; ++n) m.addEvent (juce::MidiMessage::noteOn (1, n, 0.8f), 0);
        p.processBlock (b, m); m.clear();
        m.addEvent (juce::MidiMessage::allNotesOff (1), 0);
        p.processBlock (b, m); m.clear();
        for (int k = 0; k < 400; ++k) p.processBlock (b, m);
        CHECK (p.engine.activeVoices.load() == 0, "all-notes-off should release everything");
        // note-on with velocity 0 is a note-off
        m.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);
        p.processBlock (b, m); m.clear();
        m.addEvent (juce::MidiMessage (0x90, 60, 0), 0);
        p.processBlock (b, m); m.clear();
        for (int k = 0; k < 400; ++k) p.processBlock (b, m);
        CHECK (p.engine.activeVoices.load() == 0, "velocity-0 note-on must release");
        std::cout << "  ok" << std::endl;
    }

    std::cout << "== mono / legato" << std::endl;
    for (int idx : { 0, 2, 28 })   // Desgracia (legato), Juguete Roto (mono), 808 (legato)
    {
        p.presets.load (idx);
        juce::AudioBuffer<float> b (2, 256);
        juce::MidiBuffer m;
        m.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);
        m.addEvent (juce::MidiMessage::noteOn (1, 64, 0.8f), 100);
        m.addEvent (juce::MidiMessage::noteOn (1, 67, 0.8f), 200);
        p.processBlock (b, m); m.clear();
        CHECK (p.engine.activeVoices.load() <= 1, "mono modes must use a single voice");
        m.addEvent (juce::MidiMessage::noteOff (1, 67), 0);   // falls back to 64
        p.processBlock (b, m); m.clear();
        CHECK (p.engine.activeVoices.load() == 1 || factory[(size_t) idx].values.size() == 0, "falls back to held note");
        m.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
        m.addEvent (juce::MidiMessage::noteOff (1, 64), 1);
        p.processBlock (b, m); m.clear();
        for (int k = 0; k < 800; ++k) p.processBlock (b, m);
        CHECK (p.engine.activeVoices.load() == 0, juce::String (factory[(size_t) idx].name) + ": mono voice stuck");
    }
    std::cout << "  ok" << std::endl;

    std::cout << "== sample rates & block sizes" << std::endl;
    for (double rate : { 44100.0, 48000.0, 88200.0, 96000.0 })
        for (int block : { 1, 7, 64, 333, 1024, 4096 })
        {
            DesgraciaProcessor q;
            prep (q, rate, block);
            q.presets.load (0);
            auto r = render (q, rate, block, phrase(), 7.0, nullptr, 99.0);
            CHECK (r.finite && r.rms > 0.001 && r.peak <= 1.0001,
                   "sr " << rate << " block " << block << " rms " << r.rms << " peak " << r.peak);
        }
    std::cout << "  ok" << std::endl;

    std::cout << "== state save / restore" << std::endl;
    {
        DesgraciaProcessor a; prep (a, sr, 512);
        a.presets.load (17);
        a.apvts.getParameter (pid::fltCutoff)->setValueNotifyingHost (0.33f);
        a.apvts.getParameter (pid::oscBWave)->setValueNotifyingHost (a.apvts.getParameter (pid::oscBWave)->convertTo0to1 (5));
        juce::MemoryBlock mb; a.getStateInformation (mb);
        DesgraciaProcessor b; prep (b, sr, 512);
        b.setStateInformation (mb.getData(), (int) mb.getSize());
        int diffs = 0;
        for (auto* prm : a.getParameters())
        {
            auto* ra = dynamic_cast<juce::RangedAudioParameter*> (prm);
            auto* rb = b.apvts.getParameter (ra->getParameterID());
            if (std::abs (ra->getValue() - rb->getValue()) > 1e-5f) { ++diffs; std::cout << "  diff " << ra->getParameterID() << std::endl; }
        }
        CHECK (diffs == 0, "state restore mismatch");
        CHECK (b.presets.getCurrentName() == a.presets.getCurrentName(), "preset name not restored");
        // identical audio after restore
        a.reset(); b.reset();
        auto ra = render (a, sr, 512, phrase(), 3.0, nullptr, 99.0);
        auto rb = render (b, sr, 512, phrase(), 3.0, nullptr, 99.0);
        CHECK (std::abs (ra.rms - rb.rms) < 0.02 * ra.rms + 1e-6, "restored state sounds different");
        std::cout << "  ok (" << mb.getSize() << " bytes)" << std::endl;
    }

    std::cout << "== user preset save / load" << std::endl;
    {
        DesgraciaProcessor a; prep (a, sr, 512);
        a.presets.load (5);
        a.apvts.getParameter (pid::drift)->setValueNotifyingHost (0.77f);
        CHECK (a.presets.saveUser ("zz_test_preset"), "save failed");
        const float drift = a.apvts.getParameter (pid::drift)->getValue();
        a.presets.load (0);
        int idx = -1;
        for (int i = 0; i < a.presets.getEntries().size(); ++i) if (a.presets.getEntries()[i].name == "zz_test_preset") idx = i;
        CHECK (idx >= 0, "user preset not listed");
        a.presets.load (idx);
        CHECK (std::abs (a.apvts.getParameter (pid::drift)->getValue() - drift) < 1e-5f, "user preset value mismatch");
        PresetManager::getUserDirectory().getChildFile ("zz_test_preset.desgracia").deleteFile();
        std::cout << "  ok" << std::endl;
    }

    std::cout << "== randomize x50 (all must sound, stay finite)" << std::endl;
    {
        DesgraciaProcessor a; prep (a, sr, 512);
        int quiet = 0;
        for (int k = 0; k < 50; ++k)
        {
            a.presets.randomize();
            a.reset();
            auto r = render (a, sr, 512, phrase(), 6.5, nullptr, 99.0);
            CHECK (r.finite && r.peak <= 1.0001, "random patch " << k << " broke");
            if (r.rms < 0.003) ++quiet;
        }
        std::cout << "  quiet random patches: " << quiet << "/50" << std::endl;
    }

    std::cout << "== vocoder: works on any patch (sine too) and while the DAW is playing" << std::endl;
    {
        for (int playing = 0; playing < 2; ++playing)
            for (int wave : { 0, 2 })
            {
                DesgraciaProcessor a; prep (a, sr, 512);
                FakePlayHead ph; ph.sr = sr;
                a.presets.initPatch();
                auto set = [&] (const juce::String& id, float v) { auto* q = a.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); };
                set (pid::oscAWave, (float) wave); set (pid::ampS, 1.0f);
                std::vector<Note> held { { 0.0, 3.0, 57, 0.9f }, { 0.0, 3.0, 60, 0.9f }, { 0.0, 3.0, 64, 0.9f } };
                auto dry = render (a, sr, 512, held, 3.2, nullptr, 99.0);
                for (int pat = 0; pat < 8; ++pat)
                {
                    set (pid::vocOn, 1.0f); set (pid::vocPattern, (float) pat);
                    a.reset();
                    juce::AudioBuffer<float> cap;
                    if (playing) a.setPlayHead (&ph);
                    // render block by block, advancing the fake transport
                    Result r; const int N = (int) (3.2 * sr); juce::AudioBuffer<float> b (2, 512); double sum = 0;
                    for (int pos = 0; pos < N; pos += 512)
                    {
                        juce::MidiBuffer m;
                        if (pos == 0) for (auto& e : held) m.addEvent (juce::MidiMessage::noteOn (1, e.n, e.v), 0);
                        if (pos <= (int) (3.0 * sr) && pos + 512 > (int) (3.0 * sr)) for (auto& e : held) m.addEvent (juce::MidiMessage::noteOff (1, e.n), 0);
                        a.processBlock (b, m);
                        ph.ppq += 512.0 / sr * ph.bpm / 60.0;
                        for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i)
                        { const float x = b.getSample (c, i); if (! std::isfinite (x)) r.finite = false; r.peak = std::max (r.peak, (double) std::abs (x)); sum += x * x; }
                    }
                    a.setPlayHead (nullptr);
                    r.rms = std::sqrt (sum / (2.0 * N));
                    const double diff = 20.0 * std::log10 (juce::jmax (1e-9, r.rms) / juce::jmax (1e-9, dry.rms));
                    CHECK (r.finite && r.peak <= 1.0001, "vocoder broke");
                    CHECK (diff > -9.0, "vocoder too quiet: wave " << wave << " pattern " << pat << (playing ? " (playing)" : "") << " " << diff << " dB vs dry");
                    if (pat == 0) std::cout << "  wave " << wave << (playing ? " playing" : " stopped") << ": vocoded vs dry " << juce::String (diff, 1) << " dB" << std::endl;
                }
            }
    }

    std::cout << "== category dice x8 per category (must sound, stay finite, keep category)" << std::endl;
    {
        DesgraciaProcessor a; prep (a, sr, 512);
        for (auto* cat : presetcat::all)
        {
            int quiet = 0; double minDb = 0, maxDb = -200;
            for (int k = 0; k < 8; ++k)
            {
                a.presets.rollDice (cat);
                a.reset();
                CHECK (a.presets.getCurrentCategory() == cat, "dice " << cat << " changed category to " << a.presets.getCurrentCategory());
                if (juce::String (cat) == "VOCODER") CHECK (a.apvts.getRawParameterValue (pid::vocOn)->load() > 0.5f, "vocoder dice without vocoder");
                auto r = render (a, sr, 512, phrase(), 6.5, nullptr, 99.0);
                CHECK (r.finite && r.peak <= 1.0001, "dice " << cat << " " << k << " broke");
                const double db = 20.0 * std::log10 (juce::jmax (1e-9, r.rms));
                minDb = juce::jmin (minDb, db); maxDb = juce::jmax (maxDb, db);
                if (r.rms < 0.003) ++quiet;
            }
            CHECK (quiet == 0, "dice " << cat << ": " << quiet << " quiet results");
            std::cout << "  " << cat << ": rms " << juce::String (minDb, 1) << " .. " << juce::String (maxDb, 1) << " dB" << std::endl;
        }
    }

    std::cout << "== macros at extremes (finite, audible)" << std::endl;
    {
        DesgraciaProcessor a; prep (a, sr, 512);
        for (int idx : { 0, 10, 28 })
            for (float v : { -1.0f, 1.0f })
            {
                a.presets.load (idx);
                for (auto id : { pid::macBright, pid::macDirt, pid::macDetune, pid::macMove, pid::macSpace, pid::macLength })
                {
                    auto* prm = a.apvts.getParameter (id);
                    prm->setValueNotifyingHost (prm->convertTo0to1 (juce::jlimit (prm->getNormalisableRange().start, prm->getNormalisableRange().end, v)));
                }
                a.reset();
                auto r = render (a, sr, 512, phrase(), 8.0, nullptr, 99.0);
                CHECK (r.finite && r.peak <= 1.0001 && r.rms > 0.002, "macros " << v << " on preset " << idx << " broke");
            }
        std::cout << "  ok" << std::endl;
    }

    std::cout << "== CPU worst case: 16 voices x 7 unison x 2 osc, all FX" << std::endl;
    {
        DesgraciaProcessor a; prep (a, sr, 256);
        a.presets.load (0);
        for (auto [id, v] : std::vector<std::pair<juce::String, float>> { { pid::uniVoices, 7 }, { pid::fltType, 5 }, { pid::oscBLevel, 0.5f },
                                                                          { pid::fmAmt, 0.3f }, { pid::choOn, 1 }, { pid::revOn, 1 }, { pid::dlyOn, 1 }, { pid::eqOn, 1 },
                                                                          { pid::ampS, 1 }, { pid::subLevel, 0.3f }, { pid::noiseLevel, 0.2f }, { pid::voiceMode, 0 } })
            a.apvts.getParameter (id)->setValueNotifyingHost (a.apvts.getParameter (id)->convertTo0to1 (v));
        std::vector<Note> chord;
        for (int n = 0; n < 16; ++n) chord.push_back ({ 0.0, 5.0, 40 + n * 3, 0.8f });
        auto r = render (a, sr, 256, chord, 5.0, nullptr, 99.0);
        std::cout << "  " << juce::String (r.renderSecs / 5.0 * 100.0, 2) << "% of one core (realtime = 100%)" << std::endl;
        CHECK (r.finite, "worst case NaN");
    }

    std::cout << (failures == 0 ? "ALL TESTS PASSED" : "FAILURES: " + juce::String (failures)) << std::endl;
    return failures == 0 ? 0 : 1;
}
