// pokitto_compat/PokittoLibAudio.h — compat pour <LibAudio> (Karateka_Pokitto).
//
// Usage reel observe (2 formes) :
//   auto &music = Audio::play<1>(Sounds::sfx_karate08);     // tableau embarque, taille deduite du type
//   File soundEffectFile;
//   if (soundEffectFile.openRO("music/karate09.raw"))
//       auto &music = Audio::play<1>(soundEffectFile);       // fichier SD
//
// "File" est fournie par le vrai SDK Pokitto (LibAudio), absente des sources
// du jeu -- comme palettePico/fontC64/fontTight. Reimplementee ici en
// s'appuyant sur gb_audio_track_wav (meme SDK que Pokitto::Sound).
// Tout est inline (variables 'inline' C++17) pour rester dans un seul
// fichier header, sans gestion d'instanciation explicite de template.
#pragma once
#include "aka_runtime/aka_runtime.h"
#include "pokitto_compat/PokittoSound.h"
#include "gb_audio_track_wav.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>

class File {
public:
    bool openRO(const char* path) {
        // BUG TROUVE ET CORRIGE : soundEffectFile est un membre PERSISTANT
        // de Game (pas une variable temporaire), reutilise a chaque appel.
        // m_f etait ecrase par le nouveau fopen() SANS FERMER le precedent
        // -- fuite d'un descripteur de fichier a chaque ouverture, jusqu'a
        // epuisement du systeme ("no free file descriptors").
        close();
        snprintf(m_path, sizeof m_path, "%s/%s", akaRuntime.gamePath(), path);
        // Les .raw (audio) sont convertis en .wav avec un vrai en-tete RIFF
        // pour Audio::play(File&) ; les autres extensions (.img, images
        // brutes lues via read()) passent inchangees.
        size_t n = strlen(m_path);
        if (n > 4 && strcmp(m_path + n - 4, ".raw") == 0) strcpy(m_path + n - 4, ".wav");
        m_f = fopen(m_path, "rb");
        ESP_LOGI("libaudio", "File::openRO(%s) -> %s", m_path, m_f ? "ok" : "ECHEC");
        return m_f != nullptr;
    }
    // Lecture brute directe (utilisee pour charger des images depuis la SD,
    // pas seulement pour l'audio -- trouve par analyse, Karateka).
    size_t read(void* buffer, size_t size) {
        return m_f ? fread(buffer, 1, size, m_f) : 0;
    }
    void close() { if (m_f) { fclose(m_f); m_f = nullptr; } }
    ~File() { close(); }
    const char* path() const { return m_path; }
private:
    char m_path[160] = {0};
    FILE* m_f = nullptr;
};

namespace Audio {

struct TrackHandle {
    bool isPlaying() const { return track ? track->is_playing() : false; }
    void stop() { if (track) track->stop_playing(); }
    void setLoop(bool loop) { loopEnabled = loop; }   // trouve par analyse (Karateka) -- boucle non geree par gb_audio_track_wav, memorise sans effet pour l'instant
    gb_audio_track_wav* track = nullptr;
    bool loopEnabled = false;
};

// 2 voies (N=0,1 vues dans le code) ; chaque voie a sa propre piste + son
// tampon de conversion 8bit->16bit reutilisable (PSRAM).
inline gb_audio_track_wav s_libaudio_tracks[2];
inline TrackHandle        s_libaudio_handles[2];
inline int16_t*           s_libaudio_convbuf[2] = {nullptr, nullptr};
inline size_t             s_libaudio_convcap[2] = {0, 0};
inline bool               s_libaudio_registered = false;

inline void ensure_registered() {
    // Enregistrement paresseux aupres du lecteur partage (Pokitto::Sound)
    // au premier usage -- evite une dependance d'ordre d'initialisation
    // entre modules.
    // BUG TROUVE ET CORRIGE : les pistes LibAudio n'etaient JAMAIS ajoutees
    // au lecteur partage (s_player, dans PokittoSound.cpp) -- elles
    // jouaient dans le vide (aucune erreur, mais silence total, car seul
    // s_player.pool() alimente reellement la sortie audio).
    if (s_libaudio_registered) return;
    s_libaudio_registered = true;
    s_libaudio_handles[0].track = &s_libaudio_tracks[0];
    s_libaudio_handles[1].track = &s_libaudio_tracks[1];
    Pokitto::Sound::registerTrack(&s_libaudio_tracks[0], 1.0f);
    Pokitto::Sound::registerTrack(&s_libaudio_tracks[1], 1.0f);
    ESP_LOGI("libaudio", "pistes LibAudio enregistrees aupres du lecteur partage");
}

inline int16_t* conv_buffer(int n, size_t need_samples) {
    if (s_libaudio_convcap[n] < need_samples) {
        if (s_libaudio_convbuf[n]) heap_caps_free(s_libaudio_convbuf[n]);
        s_libaudio_convbuf[n] = (int16_t*)heap_caps_malloc(need_samples * sizeof(int16_t), MALLOC_CAP_SPIRAM);
        s_libaudio_convcap[n] = s_libaudio_convbuf[n] ? need_samples : 0;
    }
    return s_libaudio_convbuf[n];
}

template<int N, size_t L>
TrackHandle& play(const uint8_t (&data)[L]) {
    ensure_registered();
    ESP_LOGI("libaudio", "play<%d>(tableau embarque, %u octets)", N, (unsigned)L);
    int16_t* buf = conv_buffer(N, L);
    if (buf) {
        for (size_t i = 0; i < L; ++i) buf[i] = (int16_t)(((int)data[i] - 128) << 8);
        s_libaudio_tracks[N].play_raw(buf, L);
    } else {
        ESP_LOGE("libaudio", "play<%d>: allocation PSRAM echouee", N);
    }
    return s_libaudio_handles[N];
}
// (uint8_t est un typedef de unsigned char -- une seule surcharge suffit,
// la seconde etait une redefinition, pas une vraie surcharge distincte)

template<int N>
TrackHandle& play(File& f) {
    ensure_registered();
    ESP_LOGI("libaudio", "play<%d>(fichier) -> %s", N, f.path());
    int r = s_libaudio_tracks[N].play_wav(f.path());
    ESP_LOGI("libaudio", "play<%d>: play_wav -> %d", N, r);
    return s_libaudio_handles[N];
}

// Audio::Sink<CHANNELS, FREQ> : declare dans le main() d'origine (initialise
// le sous-systeme audio reel du Pokitto) mais jamais appele par la logique
// de jeu elle-meme -- un type vide suffit ici (l'amorcage reel se fait via
// Pokitto::Sound::begin(), deja cable dans Core::begin()).
template<int CHANNELS, int FREQ>
struct Sink { Sink() {} };

} // namespace Audio
