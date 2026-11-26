#include "FactoryPresets.h"

// Sound design notes (from the reference analysis):
//  * energy lives in 250 Hz - 1 kHz, mids are king, highs are dry & small
//  * strong 2nd harmonic in the toy/pluck tones (octave-stacked, hollow)
//  * odd-heavy, nasal pulse tones on the main lead layer
//  * tiny pure beeps around 2.8 kHz with a fifth on top
//  * pitch is never perfectly straight: +-10..40 cents wobble, short glides
//  * 808/sub with clear 2nd + 3rd harmonics (saturated), mostly mono
//
// waves: 0 SINE 1 TRI 2 SAW 3 PULSE 4 TOY 5 FORMANT 6 BUZZ 7 METAL
// filter: 0 LP12 1 LP24 2 BP 3 HP 4 NOTCH 5 VOWEL
// lfo shape: 0 SINE 1 TRI 2 SAW 3 SQR 4 S&H 5 DRIFT
// lfo dest: 0 PITCH 1 CUTOFF 2 SHAPE 3 FM 4 AMP 5 PAN 6 B-PITCH
// divs: 0 1/1 1/2 1/4 1/4T 1/8 1/8T 1/8D 1/16 1/16T 1/32
// voice: 0 POLY 1 MONO 2 LEGATO   drive: 0 SOFT 1 HARD 2 FOLD 3 TUBE

const std::vector<FactoryPreset>& getFactoryPresets()
{
    static const std::vector<FactoryPreset> presets = {

    //=========================================================== ICONIC LEADS
    { "Desgracia", "ICONIC LEADS", "que desgracia...", {
        { "oscA_wave", 5 }, { "oscA_shape", 0.42f }, { "oscA_level", 0.8f },
        { "oscB_wave", 3 }, { "oscB_shape", 0.55f }, { "oscB_level", 0.38f }, { "oscB_fine", 9 },
        { "drift", 0.45f },
        { "flt_type", 1 }, { "flt_cutoff", 2600 }, { "flt_reso", 0.42f }, { "flt_env", 0.22f }, { "flt_key", 0.4f },
        { "fenv_d", 0.25f }, { "fenv_s", 0.2f },
        { "amp_a", 0.003f }, { "amp_d", 0.5f }, { "amp_s", 0.75f }, { "amp_r", 0.12f },
        { "voice_mode", 2 }, { "glide", 0.06f },
        { "lfo1_amt", 0.17f }, { "lfo1_rate", 5.2f }, { "lfo1_delay", 0.35f },
        { "drv_on", 1 }, { "drv_type", 3 }, { "drv_amt", 0.3f },
        { "crs_on", 1 }, { "crs_bits", 9 }, { "crs_rate", 22000 }, { "crs_mix", 0.45f },
        { "dly_on", 1 }, { "dly_div", 6 }, { "dly_fb", 0.28f }, { "dly_mix", 0.12f }, { "dly_tone", 2500 },
        { "master", -8.5 } } },

    { "Nariz de Lata", "ICONIC LEADS", "nasal, hueco, de lata", {
        { "oscA_wave", 3 }, { "oscA_shape", 0.62f }, { "oscA_level", 0.85f },
        { "oscB_wave", 3 }, { "oscB_shape", 0.3f }, { "oscB_level", 0.35f }, { "oscB_oct", 1 }, { "oscB_fine", -7 },
        { "drift", 0.3f },
        { "flt_type", 2 }, { "flt_cutoff", 1250 }, { "flt_reso", 0.5f }, { "flt_env", 0.18f }, { "flt_key", 0.7f },
        { "fenv_d", 0.18f },
        { "amp_a", 0.002f }, { "amp_s", 0.85f }, { "amp_r", 0.09f },
        { "voice_mode", 2 }, { "glide", 0.045f },
        { "lfo1_amt", 0.13f }, { "lfo1_rate", 6.0f }, { "lfo1_delay", 0.25f },
        { "drv_on", 1 }, { "drv_type", 1 }, { "drv_amt", 0.25f },
        { "eq_on", 1 }, { "eq_low", -4 }, { "eq_mid", 3 }, { "eq_midf", 1100 },
        { "master", -8.8 } } },

    { "Juguete Roto", "ICONIC LEADS", "lo encontre en la basura", {
        { "oscA_wave", 4 }, { "oscA_shape", 0.55f }, { "oscA_level", 0.85f },
        { "oscB_wave", 0 }, { "oscB_oct", 1 }, { "oscB_level", 0.3f },
        { "drift", 0.5f },
        { "flt_type", 0 }, { "flt_cutoff", 4800 }, { "flt_reso", 0.15f },
        { "amp_a", 0.001f }, { "amp_d", 0.45f }, { "amp_s", 0.6f }, { "amp_r", 0.1f },
        { "voice_mode", 1 }, { "glide", 0.035f },
        { "lfo1_amt", 0.11f }, { "lfo1_rate", 7.5f }, { "lfo1_shape", 1 },
        { "crs_on", 1 }, { "crs_bits", 7 }, { "crs_rate", 14000 }, { "crs_mix", 0.6f },
        { "master", -2.6 } } },

    { "Alarma Hueca", "ICONIC LEADS", "hueca por dentro", {
        { "oscA_wave", 2 }, { "oscA_shape", 0.75f }, { "oscA_level", 0.8f },
        { "uni_voices", 3 }, { "uni_detune", 0.16f }, { "uni_spread", 0.35f }, { "drift", 0.25f },
        { "flt_type", 2 }, { "flt_cutoff", 1500 }, { "flt_reso", 0.58f }, { "flt_key", 0.6f },
        { "amp_a", 0.004f }, { "amp_s", 0.9f }, { "amp_r", 0.15f },
        { "voice_mode", 2 }, { "glide", 0.08f },
        { "lfo2_dest", 1 }, { "lfo2_amt", 0.18f }, { "lfo2_rate", 0.3f }, { "lfo2_shape", 5 },
        { "drv_on", 1 }, { "drv_type", 1 }, { "drv_amt", 0.32f },
        { "dly_on", 1 }, { "dly_div", 4 }, { "dly_fb", 0.22f }, { "dly_mix", 0.1f },
        { "master", -4.9 } } },

    { "Vocoder Pobre", "ICONIC LEADS", "a-e-i-o-u... mal", {
        { "oscA_wave", 6 }, { "oscA_shape", 0.35f }, { "oscA_level", 0.9f },
        { "oscB_wave", 2 }, { "oscB_level", 0.35f }, { "oscB_fine", 12 },
        { "drift", 0.35f },
        { "flt_type", 5 }, { "flt_cutoff", 700 }, { "flt_reso", 0.55f }, { "flt_env", 0.3f }, { "flt_key", 0.0f },
        { "fenv_a", 0.05f }, { "fenv_d", 0.6f }, { "fenv_s", 0.3f },
        { "amp_a", 0.004f }, { "amp_s", 0.9f }, { "amp_r", 0.12f },
        { "voice_mode", 2 }, { "glide", 0.07f },
        { "lfo2_dest", 1 }, { "lfo2_amt", 0.3f }, { "lfo2_sync", 1 }, { "lfo2_div", 1 }, { "lfo2_shape", 1 },
        { "lfo1_amt", 0.12f }, { "lfo1_delay", 0.4f },
        { "drv_on", 1 }, { "drv_amt", 0.35f },
        { "master", -1.7 } } },

    { "Mosquito", "ICONIC LEADS", "bzzzzz", {
        { "oscA_wave", 6 }, { "oscA_shape", 0.65f }, { "oscA_level", 0.8f },
        { "oscB_wave", 6 }, { "oscB_shape", 0.4f }, { "oscB_level", 0.4f }, { "oscB_fine", -14 },
        { "drift", 0.4f },
        { "flt_type", 2 }, { "flt_cutoff", 1900 }, { "flt_reso", 0.25f }, { "flt_key", 0.8f },
        { "amp_a", 0.002f }, { "amp_s", 0.8f }, { "amp_r", 0.08f },
        { "voice_mode", 1 }, { "glide", 0.03f },
        { "lfo1_amt", 0.16f }, { "lfo1_rate", 8.5f },
        { "crs_on", 1 }, { "crs_bits", 11 }, { "crs_rate", 18000 }, { "crs_mix", 0.5f },
        { "master", 1.9 } } },

    { "Silbido Alien", "ICONIC LEADS", "silbando desde otro planeta", {
        { "oscA_wave", 0 }, { "oscA_shape", 0.22f }, { "oscA_level", 0.85f },
        { "oscB_wave", 1 }, { "oscB_oct", 1 }, { "oscB_level", 0.12f },
        { "drift", 0.6f },
        { "flt_type", 0 }, { "flt_cutoff", 6000 },
        { "amp_a", 0.015f }, { "amp_s", 0.85f }, { "amp_r", 0.25f },
        { "voice_mode", 2 }, { "glide", 0.14f },
        { "lfo1_amt", 0.21f }, { "lfo1_rate", 4.6f }, { "lfo1_delay", 0.5f },
        { "dly_on", 1 }, { "dly_div", 6 }, { "dly_fb", 0.4f }, { "dly_mix", 0.2f }, { "dly_tone", 3000 },
        { "rev_on", 1 }, { "rev_size", 0.4f }, { "rev_mix", 0.12f },
        { "master", -3.4 } } },

    { "Cuadrada Barata", "ICONIC LEADS", "seca. barata. perfecta.", {
        { "oscA_wave", 3 }, { "oscA_level", 0.8f },
        { "oscB_wave", 3 }, { "oscB_level", 0.3f }, { "oscB_fine", 6 },
        { "drift", 0.15f },
        { "flt_type", 1 }, { "flt_cutoff", 3400 }, { "flt_reso", 0.12f },
        { "amp_a", 0.001f }, { "amp_s", 0.85f }, { "amp_r", 0.05f },
        { "voice_mode", 1 }, { "glide", 0.02f },
        { "crs_on", 1 }, { "crs_bits", 10 }, { "crs_rate", 30000 }, { "crs_mix", 0.6f },
        { "master", -7.3 } } },

    //=========================================================== WEIRD PLUCKS
    { "Gota de Vidrio", "WEIRD PLUCKS", "plin.", {
        { "oscA_wave", 7 }, { "oscA_shape", 0.25f }, { "oscA_level", 0.75f },
        { "oscB_wave", 0 }, { "oscB_oct", 2 }, { "oscB_level", 0.25f },
        { "drift", 0.15f },
        { "flt_type", 0 }, { "flt_cutoff", 3500 }, { "flt_env", 0.35f }, { "flt_key", 0.5f },
        { "fenv_d", 0.25f },
        { "amp_a", 0.0007f }, { "amp_d", 0.45f }, { "amp_s", 0.0f }, { "amp_r", 0.35f },
        { "dly_on", 1 }, { "dly_div", 4 }, { "dly_fb", 0.3f }, { "dly_mix", 0.18f },
        { "rev_on", 1 }, { "rev_size", 0.35f }, { "rev_mix", 0.1f },
        { "master", -1.1 } } },

    { "Pluck Pixel", "WEIRD PLUCKS", "8 bits de tristeza", {
        { "oscA_wave", 4 }, { "oscA_shape", 0.7f }, { "oscA_level", 0.85f },
        { "oscB_wave", 3 }, { "oscB_level", 0.2f }, { "oscB_oct", -1 },
        { "penv_amt", 12 }, { "penv_decay", 0.018f },
        { "drift", 0.25f },
        { "flt_type", 0 }, { "flt_cutoff", 5000 },
        { "amp_a", 0.0005f }, { "amp_d", 0.22f }, { "amp_s", 0.0f }, { "amp_r", 0.12f },
        { "crs_on", 1 }, { "crs_bits", 6 }, { "crs_rate", 11000 }, { "crs_mix", 0.55f },
        { "master", -2.1 } } },

    { "Pinchazo", "WEIRD PLUCKS", "auch", {
        { "oscA_wave", 3 }, { "oscA_shape", 0.4f }, { "oscA_level", 0.8f },
        { "oscB_wave", 2 }, { "oscB_level", 0.4f }, { "oscB_fine", 11 },
        { "drift", 0.2f },
        { "flt_type", 1 }, { "flt_cutoff", 280 }, { "flt_reso", 0.55f }, { "flt_env", 0.62f }, { "flt_key", 0.5f },
        { "fenv_d", 0.16f },
        { "amp_a", 0.0007f }, { "amp_d", 0.35f }, { "amp_s", 0.0f }, { "amp_r", 0.15f },
        { "drv_on", 1 }, { "drv_type", 0 }, { "drv_amt", 0.25f },
        { "dly_on", 1 }, { "dly_div", 7 }, { "dly_fb", 0.25f }, { "dly_mix", 0.12f },
        { "master", -3.8 } } },

    { "Bip Bip", "WEIRD PLUCKS", "bip.", {
        { "oscA_wave", 0 }, { "oscA_level", 0.8f },
        { "oscB_wave", 0 }, { "oscB_semi", 7 }, { "oscB_level", 0.4f },
        { "drift", 0.08f },
        { "flt_type", 0 }, { "flt_cutoff", 20000 },
        { "amp_a", 0.0008f }, { "amp_d", 0.09f }, { "amp_s", 0.0f }, { "amp_r", 0.05f }, { "amp_vel", 0.2f },
        { "master", -2.4 } } },

    { "Marimba de Plastico", "WEIRD PLUCKS", "toc toc", {
        { "oscA_wave", 0 }, { "oscA_level", 0.85f },
        { "oscB_wave", 0 }, { "oscB_oct", 2 }, { "oscB_level", 0.0f },
        { "fm_amt", 0.05f }, { "fm_env", 0.55f },
        { "fenv_d", 0.09f },
        { "flt_type", 0 }, { "flt_cutoff", 9000 },
        { "amp_a", 0.0005f }, { "amp_d", 0.5f }, { "amp_s", 0.0f }, { "amp_r", 0.25f },
        { "drift", 0.12f },
        { "drv_on", 1 }, { "drv_type", 3 }, { "drv_amt", 0.18f },
        { "master", -6.5 } } },

    //=========================================================== DIGITAL KEYS
    { "Teclado de Feria", "DIGITAL KEYS", "comprado en once", {
        { "oscA_wave", 4 }, { "oscA_shape", 0.35f }, { "oscA_level", 0.7f },
        { "oscB_wave", 3 }, { "oscB_shape", 0.5f }, { "oscB_level", 0.35f }, { "oscB_oct", 1 },
        { "drift", 0.3f },
        { "flt_type", 0 }, { "flt_cutoff", 3800 }, { "flt_env", 0.15f },
        { "amp_a", 0.001f }, { "amp_d", 1.2f }, { "amp_s", 0.35f }, { "amp_r", 0.2f },
        { "cho_on", 1 }, { "cho_mix", 0.4f }, { "cho_rate", 0.8f },
        { "crs_on", 1 }, { "crs_bits", 12 }, { "crs_rate", 22050 }, { "crs_mix", 0.7f },
        { "master", -3.8 } } },

    { "Organito Triste", "DIGITAL KEYS", "domingo a la tarde", {
        { "oscA_wave", 1 }, { "oscA_level", 0.8f },
        { "oscB_wave", 3 }, { "oscB_oct", 1 }, { "oscB_level", 0.25f },
        { "sub_level", 0.35f }, { "sub_wave", 2 },
        { "drift", 0.35f },
        { "flt_type", 0 }, { "flt_cutoff", 2600 },
        { "amp_a", 0.008f }, { "amp_s", 1.0f }, { "amp_r", 0.18f },
        { "lfo1_amt", 0.09f }, { "lfo1_rate", 5.0f },
        { "cho_on", 1 }, { "cho_mix", 0.5f }, { "cho_depth", 0.4f },
        { "master", -6.2 } } },

    { "Piano Desafinado", "DIGITAL KEYS", "nadie lo afino desde 1994", {
        { "oscA_wave", 0 }, { "oscA_shape", 0.2f }, { "oscA_level", 0.75f },
        { "oscB_wave", 7 }, { "oscB_shape", 0.3f }, { "oscB_oct", 1 }, { "oscB_level", 0.3f },
        { "fm_env", 0.25f }, { "fenv_d", 0.2f },
        { "uni_voices", 2 }, { "uni_detune", 0.14f }, { "uni_spread", 0.4f }, { "drift", 0.55f },
        { "flt_type", 0 }, { "flt_cutoff", 5000 }, { "flt_env", 0.2f },
        { "amp_a", 0.0007f }, { "amp_d", 1.1f }, { "amp_s", 0.08f }, { "amp_r", 0.3f },
        { "rev_on", 1 }, { "rev_size", 0.3f }, { "rev_mix", 0.1f },
        { "master", -3.1 } } },

    { "Rhodes de Lata", "DIGITAL KEYS", "un rhodes de cotillon", {
        { "oscA_wave", 0 }, { "oscA_level", 0.85f },
        { "oscB_wave", 0 }, { "oscB_oct", 1 }, { "oscB_level", 0.0f },
        { "fm_amt", 0.08f }, { "fm_env", 0.35f }, { "fenv_d", 0.5f }, { "fenv_s", 0.1f },
        { "flt_type", 0 }, { "flt_cutoff", 7000 },
        { "amp_a", 0.001f }, { "amp_d", 1.6f }, { "amp_s", 0.25f }, { "amp_r", 0.3f },
        { "drift", 0.2f },
        { "lfo2_dest", 4 }, { "lfo2_amt", 0.25f }, { "lfo2_rate", 4.2f },
        { "drv_on", 1 }, { "drv_type", 3 }, { "drv_amt", 0.28f },
        { "cho_on", 1 }, { "cho_mix", 0.3f },
        { "master", -9.1 } } },

    //=========================================================== METALLIC / FM
    { "Campana Oxidada", "METALLIC / FM", "suena a domingo de misa", {
        { "oscA_wave", 0 }, { "oscA_level", 0.8f },
        { "oscB_wave", 0 }, { "oscB_semi", 6 }, { "oscB_fine", 17 }, { "oscB_level", 0.0f },
        { "fm_amt", 0.12f }, { "fm_env", 0.45f }, { "fenv_d", 0.9f },
        { "flt_type", 0 }, { "flt_cutoff", 9000 },
        { "amp_a", 0.0005f }, { "amp_d", 2.4f }, { "amp_s", 0.0f }, { "amp_r", 0.9f },
        { "drift", 0.1f },
        { "rev_on", 1 }, { "rev_size", 0.55f }, { "rev_mix", 0.18f }, { "rev_damp", 0.6f },
        { "master", -5.1 } } },

    { "Lata FM", "METALLIC / FM", "tin tin tin", {
        { "oscA_wave", 2 }, { "oscA_level", 0.75f },
        { "oscB_wave", 0 }, { "oscB_oct", 1 }, { "oscB_semi", 7 }, { "oscB_level", 0.0f },
        { "fm_amt", 0.3f }, { "fm_env", 0.25f }, { "fenv_d", 0.25f },
        { "flt_type", 1 }, { "flt_cutoff", 3600 }, { "flt_reso", 0.3f },
        { "amp_a", 0.001f }, { "amp_d", 0.6f }, { "amp_s", 0.5f }, { "amp_r", 0.12f },
        { "voice_mode", 2 }, { "glide", 0.04f },
        { "drift", 0.25f },
        { "drv_on", 1 }, { "drv_type", 2 }, { "drv_amt", 0.22f }, { "drv_mix", 0.7f },
        { "master", -1.2 } } },

    { "Chapa", "METALLIC / FM", "chapa caliente", {
        { "oscA_wave", 7 }, { "oscA_shape", 0.7f }, { "oscA_level", 0.6f },
        { "oscB_wave", 3 }, { "oscB_semi", -5 }, { "oscB_fine", 23 }, { "oscB_level", 0.15f },
        { "ring_amt", 0.55f },
        { "flt_type", 3 }, { "flt_cutoff", 380 }, { "flt_reso", 0.2f },
        { "amp_a", 0.0005f }, { "amp_d", 0.28f }, { "amp_s", 0.0f }, { "amp_r", 0.18f },
        { "drift", 0.2f },
        { "dly_on", 1 }, { "dly_div", 5 }, { "dly_fb", 0.3f }, { "dly_mix", 0.16f },
        { "master", -1.5 } } },

    { "Engranaje", "METALLIC / FM", "algo se rompio adentro", {
        { "oscA_wave", 0 }, { "oscA_level", 0.8f },
        { "oscB_wave", 0 }, { "oscB_oct", 1 }, { "oscB_semi", 3 }, { "oscB_fine", 40 }, { "oscB_level", 0.05f },
        { "fm_amt", 0.15f },
        { "lfo2_dest", 3 }, { "lfo2_amt", 0.6f }, { "lfo2_shape", 4 }, { "lfo2_sync", 1 }, { "lfo2_div", 7 },
        { "flt_type", 1 }, { "flt_cutoff", 4500 }, { "flt_reso", 0.25f },
        { "amp_a", 0.001f }, { "amp_d", 0.4f }, { "amp_s", 0.6f }, { "amp_r", 0.1f },
        { "drift", 0.2f },
        { "crs_on", 1 }, { "crs_bits", 9 }, { "crs_rate", 16000 }, { "crs_mix", 0.4f },
        { "master", -5.1 } } },

    //=========================================================== DARK TEXTURES
    { "Pasillo Vacio", "DARK TEXTURES", "no hay nadie", {
        { "oscA_wave", 2 }, { "oscA_shape", 0.5f }, { "oscA_level", 0.7f },
        { "uni_voices", 5 }, { "uni_detune", 0.3f }, { "uni_spread", 0.9f }, { "drift", 0.4f },
        { "noise_level", 0.06f }, { "noise_color", 0.9f },
        { "flt_type", 0 }, { "flt_cutoff", 1500 }, { "flt_reso", 0.18f }, { "flt_key", 0.2f },
        { "amp_a", 1.1f }, { "amp_d", 1.0f }, { "amp_s", 0.75f }, { "amp_r", 1.8f }, { "amp_vel", 0.2f },
        { "lfo2_dest", 1 }, { "lfo2_amt", 0.22f }, { "lfo2_rate", 0.12f }, { "lfo2_shape", 5 },
        { "eq_on", 1 }, { "eq_low", -7 }, { "eq_high", -2 },
        { "rev_on", 1 }, { "rev_size", 0.8f }, { "rev_damp", 0.7f }, { "rev_mix", 0.3f },
        { "width", 1.4f },
        { "master", -2.1 } } },

    { "Fantasma Digital", "DARK TEXTURES", "boo (en 8 bits)", {
        { "oscA_wave", 5 }, { "oscA_shape", 0.6f }, { "oscA_level", 0.7f },
        { "oscB_wave", 4 }, { "oscB_shape", 0.5f }, { "oscB_level", 0.3f }, { "oscB_oct", 1 }, { "oscB_fine", -18 },
        { "drift", 0.6f },
        { "flt_type", 1 }, { "flt_cutoff", 1800 }, { "flt_reso", 0.3f },
        { "amp_a", 0.45f }, { "amp_s", 0.8f }, { "amp_r", 1.2f },
        { "lfo1_amt", 0.08f }, { "lfo1_rate", 0.7f }, { "lfo1_shape", 5 },
        { "lfo2_dest", 2 }, { "lfo2_amt", 0.5f }, { "lfo2_rate", 0.25f },
        { "crs_on", 1 }, { "crs_bits", 8 }, { "crs_rate", 6000 }, { "crs_mix", 0.4f },
        { "dly_on", 1 }, { "dly_div", 2 }, { "dly_fb", 0.45f }, { "dly_mix", 0.22f }, { "dly_tone", 1800 },
        { "master", -4.1 } } },

    { "Heladera", "DARK TEXTURES", "zumbido a las 3am", {
        { "oscA_wave", 3 }, { "oscA_shape", 0.7f }, { "oscA_level", 0.4f }, { "oscA_oct", -1 },
        { "noise_level", 0.45f }, { "noise_color", 0.55f },
        { "flt_type", 2 }, { "flt_cutoff", 650 }, { "flt_reso", 0.7f }, { "flt_key", 0.5f },
        { "amp_a", 0.6f }, { "amp_s", 0.9f }, { "amp_r", 1.5f },
        { "lfo2_dest", 1 }, { "lfo2_amt", 0.35f }, { "lfo2_rate", 0.18f }, { "lfo2_shape", 5 },
        { "lfo1_dest", 4 }, { "lfo1_amt", 0.2f }, { "lfo1_rate", 12.0f },
        { "drift", 0.3f },
        { "drv_on", 1 }, { "drv_amt", 0.3f },
        { "rev_on", 1 }, { "rev_size", 0.6f }, { "rev_mix", 0.2f },
        { "master", -6.3 } } },

    //=========================================================== AMBIENT
    { "Niebla Barata", "AMBIENT", "humo de maquina", {
        { "oscA_wave", 1 }, { "oscA_shape", 0.15f }, { "oscA_level", 0.7f },
        { "oscB_wave", 0 }, { "oscB_oct", 1 }, { "oscB_fine", 8 }, { "oscB_level", 0.35f },
        { "uni_voices", 3 }, { "uni_detune", 0.22f }, { "uni_spread", 0.8f }, { "drift", 0.65f },
        { "noise_level", 0.04f }, { "noise_color", 0.8f },
        { "flt_type", 0 }, { "flt_cutoff", 2200 },
        { "amp_a", 0.9f }, { "amp_s", 0.8f }, { "amp_r", 2.2f }, { "amp_vel", 0.2f },
        { "cho_on", 1 }, { "cho_mix", 0.5f }, { "cho_depth", 0.5f }, { "cho_rate", 0.3f },
        { "rev_on", 1 }, { "rev_size", 0.75f }, { "rev_mix", 0.32f }, { "rev_damp", 0.65f },
        { "eq_on", 1 }, { "eq_low", -5 },
        { "master", -3.4 } } },

    { "Satelite", "AMBIENT", "transmitiendo... nadie escucha", {
        { "oscA_wave", 0 }, { "oscA_level", 0.75f },
        { "oscB_wave", 7 }, { "oscB_shape", 0.15f }, { "oscB_oct", 1 }, { "oscB_level", 0.15f },
        { "drift", 0.3f },
        { "flt_type", 0 }, { "flt_cutoff", 7000 },
        { "amp_a", 0.002f }, { "amp_d", 0.7f }, { "amp_s", 0.15f }, { "amp_r", 0.8f },
        { "lfo1_amt", 0.1f }, { "lfo1_rate", 3.0f },
        { "dly_on", 1 }, { "dly_div", 6 }, { "dly_fb", 0.62f }, { "dly_mix", 0.3f }, { "dly_tone", 2800 },
        { "rev_on", 1 }, { "rev_size", 0.85f }, { "rev_mix", 0.3f },
        { "width", 1.3f },
        { "master", -3.1 } } },

    { "Respira", "AMBIENT", "inhala... exhala...", {
        { "oscA_wave", 5 }, { "oscA_shape", 0.25f }, { "oscA_level", 0.85f },
        { "noise_level", 0.45f }, { "noise_color", 0.7f },
        { "uni_voices", 2 }, { "uni_detune", 0.12f }, { "uni_spread", 0.6f }, { "drift", 0.5f },
        { "flt_type", 5 }, { "flt_cutoff", 900 }, { "flt_reso", 0.3f }, { "flt_key", 0.0f },
        { "amp_a", 1.4f }, { "amp_s", 0.85f }, { "amp_r", 2.5f },
        { "lfo2_dest", 1 }, { "lfo2_amt", 0.4f }, { "lfo2_rate", 0.1f }, { "lfo2_shape", 0 },
        { "rev_on", 1 }, { "rev_size", 0.8f }, { "rev_mix", 0.35f },
        { "master", -1.4 } } },

    //=========================================================== BASS
    { "Sub Sucio", "BASS", "sub con mugre", {
        { "oscA_wave", 0 }, { "oscA_shape", 0.18f }, { "oscA_level", 0.85f }, { "oscA_oct", -1 },
        { "sub_level", 0.4f }, { "sub_oct", 0 },
        { "drift", 0.05f },
        { "flt_type", 0 }, { "flt_cutoff", 1100 }, { "flt_key", 0.0f },
        { "penv_amt", 3 }, { "penv_decay", 0.035f },
        { "amp_a", 0.001f }, { "amp_d", 1.5f }, { "amp_s", 0.8f }, { "amp_r", 0.12f }, { "amp_vel", 0.25f },
        { "voice_mode", 2 }, { "glide", 0.06f },
        { "drv_on", 1 }, { "drv_type", 3 }, { "drv_amt", 0.45f },
        { "eq_on", 1 }, { "eq_low", 2 }, { "eq_mid", 2 }, { "eq_midf", 600 },
        { "width", 0.0f },
        { "master", -9.4 } } },

    { "808 Desgracia", "BASS", "boom. desgracia.", {
        { "oscA_wave", 0 }, { "oscA_level", 0.9f }, { "oscA_oct", -1 },
        { "penv_amt", 12 }, { "penv_decay", 0.045f },
        { "drift", 0.0f },
        { "flt_type", 0 }, { "flt_cutoff", 2500 }, { "flt_key", 0.0f },
        { "amp_a", 0.0007f }, { "amp_d", 2.6f }, { "amp_s", 0.0f }, { "amp_r", 0.35f }, { "amp_vel", 0.2f },
        { "voice_mode", 2 }, { "glide", 0.07f },
        { "drv_on", 1 }, { "drv_type", 3 }, { "drv_amt", 0.55f },
        { "width", 0.0f },
        { "master", -5 } } },

    { "Cuadrado Grave", "BASS", "grave y cuadrado", {
        { "oscA_wave", 3 }, { "oscA_level", 0.7f }, { "oscA_oct", -1 },
        { "sub_level", 0.5f },
        { "flt_type", 1 }, { "flt_cutoff", 320 }, { "flt_reso", 0.32f }, { "flt_env", 0.35f }, { "flt_key", 0.3f },
        { "fenv_d", 0.22f }, { "fenv_s", 0.1f },
        { "amp_a", 0.001f }, { "amp_s", 0.85f }, { "amp_r", 0.08f },
        { "voice_mode", 2 }, { "glide", 0.03f },
        { "drift", 0.08f },
        { "drv_on", 1 }, { "drv_type", 1 }, { "drv_amt", 0.2f },
        { "width", 0.0f },
        { "master", -8.1 } } },

    { "Reese de Juguete", "BASS", "reese del chino", {
        { "oscA_wave", 2 }, { "oscA_level", 0.75f }, { "oscA_oct", -1 },
        { "uni_voices", 2 }, { "uni_detune", 0.28f }, { "uni_spread", 0.1f },
        { "sub_level", 0.45f },
        { "flt_type", 1 }, { "flt_cutoff", 520 }, { "flt_reso", 0.22f }, { "flt_key", 0.2f },
        { "amp_a", 0.002f }, { "amp_s", 0.9f }, { "amp_r", 0.1f },
        { "voice_mode", 2 }, { "glide", 0.05f },
        { "drift", 0.1f },
        { "drv_on", 1 }, { "drv_type", 0 }, { "drv_amt", 0.38f },
        { "crs_on", 1 }, { "crs_bits", 9 }, { "crs_rate", 20000 }, { "crs_mix", 0.3f },
        { "width", 0.3f },
        { "master", -3.7 } } },

    //=========================================================== EXPERIMENTAL
    { "Error 404", "EXPERIMENTAL", "no se encontro la melodia", {
        { "oscA_wave", 3 }, { "oscA_shape", 0.3f }, { "oscA_level", 0.75f },
        { "oscB_wave", 4 }, { "oscB_oct", 1 }, { "oscB_level", 0.3f },
        { "lfo1_shape", 4 }, { "lfo1_sync", 1 }, { "lfo1_div", 7 }, { "lfo1_amt", 0.35f },
        { "drift", 0.3f },
        { "flt_type", 0 }, { "flt_cutoff", 3500 },
        { "amp_a", 0.001f }, { "amp_s", 0.8f }, { "amp_r", 0.1f },
        { "voice_mode", 1 },
        { "crs_on", 1 }, { "crs_bits", 5 }, { "crs_rate", 8000 }, { "crs_mix", 0.5f },
        { "master", -6.8 } } },

    { "Lavarropas", "EXPERIMENTAL", "centrifugando", {
        { "oscA_wave", 2 }, { "oscA_level", 0.55f },
        { "oscB_wave", 1 }, { "oscB_semi", 5 }, { "oscB_fine", -30 }, { "oscB_level", 0.2f },
        { "ring_amt", 0.6f },
        { "flt_type", 1 }, { "flt_cutoff", 900 }, { "flt_reso", 0.55f },
        { "lfo2_dest", 1 }, { "lfo2_amt", 0.5f }, { "lfo2_shape", 2 }, { "lfo2_sync", 1 }, { "lfo2_div", 4 },
        { "amp_a", 0.002f }, { "amp_s", 0.85f }, { "amp_r", 0.12f },
        { "drift", 0.3f },
        { "drv_on", 1 }, { "drv_type", 2 }, { "drv_amt", 0.3f }, { "drv_mix", 0.6f },
        { "master", -3.7 } } },

    { "Grillo", "EXPERIMENTAL", "cri cri", {
        { "oscA_wave", 6 }, { "oscA_shape", 0.5f }, { "oscA_level", 0.9f }, { "oscA_oct", 1 },
        { "oscB_wave", 0 }, { "oscB_oct", 1 }, { "oscB_semi", 7 }, { "oscB_level", 0.2f },
        { "lfo1_dest", 4 }, { "lfo1_amt", 0.75f }, { "lfo1_rate", 19.0f }, { "lfo1_shape", 3 },
        { "flt_type", 2 }, { "flt_cutoff", 3500 }, { "flt_reso", 0.2f }, { "flt_key", 0.6f },
        { "amp_a", 0.005f }, { "amp_d", 0.5f }, { "amp_s", 0.4f }, { "amp_r", 0.2f },
        { "drift", 0.35f },
        { "dly_on", 1 }, { "dly_div", 5 }, { "dly_fb", 0.35f }, { "dly_mix", 0.18f },
        { "master", 1.1 } } },

    { "Caida Libre", "EXPERIMENTAL", "aaaaaaaaaa", {
        { "oscA_wave", 5 }, { "oscA_shape", 0.5f }, { "oscA_level", 0.8f },
        { "oscB_wave", 2 }, { "oscB_level", 0.3f }, { "oscB_oct", -1 },
        { "penv_amt", -24 }, { "penv_decay", 0.6f },
        { "flt_type", 1 }, { "flt_cutoff", 2500 }, { "flt_reso", 0.35f }, { "flt_env", 0.3f },
        { "fenv_d", 0.5f },
        { "amp_a", 0.001f }, { "amp_d", 0.7f }, { "amp_s", 0.0f }, { "amp_r", 0.3f },
        { "drift", 0.4f },
        { "drv_on", 1 }, { "drv_type", 1 }, { "drv_amt", 0.3f },
        { "dly_on", 1 }, { "dly_div", 4 }, { "dly_fb", 0.35f }, { "dly_mix", 0.15f },
        { "master", -5.3 } } },
    };
    return presets;
}
