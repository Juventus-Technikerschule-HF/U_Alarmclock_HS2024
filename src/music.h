#define NOTE_A2   110    // A2  = 110 Hz
#define NOTE_A2S  116    // A#2 = 116 Hz
#define NOTE_B2   123    // B2  = 123 Hz
#define NOTE_C3   130    // C3  = 130 Hz
#define NOTE_C3S  138    // C#3 = 138 Hz
#define NOTE_D3   146    // D3  = 146 Hz
#define NOTE_D3S  155    // D#3 = 155 Hz
#define NOTE_E3   164    // E3  = 164 Hz
#define NOTE_F3   174    // F3  = 174 Hz
#define NOTE_F3S  185    // F#3 = 185 Hz
#define NOTE_G3   196    // G3  = 196 Hz
#define NOTE_G3S  207    // G#3 = 207 Hz
#define NOTE_A3   220    // A3  = 220 Hz
#define NOTE_A3S  233    // A#3 = 233 Hz
#define NOTE_B3   246    // B3  = 246 Hz
#define NOTE_C4   261    // C4  = 261 Hz
#define NOTE_C4S  277    // C#4 = 277 Hz
#define NOTE_D4   293    // D4  = 293 Hz
#define NOTE_D4S  311    // D#4 = 311 Hz
#define NOTE_E4   329    // E4  = 329 Hz
#define NOTE_F4   349    // F4  = 349 Hz
#define NOTE_F4S  370    // F#4 = 370 Hz
#define NOTE_G4   392    // G4  = 392 Hz
#define NOTE_G4S  415    // G#4 = 415 Hz
#define NOTE_A4   440    // A4  = 440 Hz
#define NOTE_A4S  466    // A#4 = 466 Hz
#define NOTE_B4   493    // B4  = 493 Hz
#define NOTE_C5   523    // C5  = 523 Hz
#define NOTE_C5S  554    // C#5 = 554 Hz
#define NOTE_D5   587    // D5  = 587 Hz
#define NOTE_D5S  622    // D#5 = 622 Hz
#define NOTE_E5   659    // E5  = 659 Hz
#define NOTE_F5   698    // F5  = 698 Hz
#define NOTE_F5S  740    // F#5 = 740 Hz
#define NOTE_G5   784    // G5  = 784 Hz
#define NOTE_G5S  830    // G#5 = 830 Hz
#define NOTE_A5   880    // A5  = 880 Hz
#define NOTE_A5S  932    // A#5 = 932 Hz
#define NOTE_B5   987    // B5  = 987 Hz
#define NOTE_C6   1046   // C6  = 1046 Hz
#define NOTE_C6S  1108   // C#6 = 1108 Hz
#define NOTE_D6   1174   // D6  = 1174 Hz
#define NOTE_D6S  1244   // D#6 = 1244 Hz
#define NOTE_E6   1318   // E6  = 1318 Hz
#define NOTE_F6   1396   // F6  = 1396 Hz
#define NOTE_F6S  1480   // F#6 = 1480 Hz
#define NOTE_G6   1568   // G6  = 1568 Hz
#define NOTE_G6S  1661   // G#6 = 1661 Hz
#define NOTE_A6   1760   // A6  = 1760 Hz

typedef struct {
    uint16_t freq;
    uint16_t length_ms;
} note_t;