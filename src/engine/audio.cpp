#include <ogg/ogg.h>
#include <SDL2/SDL.h>
#include <opus/opus.h>
#include <common/assertion.h>
#include <common/raii_types.h>
#include "audio.h"

#include <fstream>
#include <cassert>
#include <cstring> // memcpy

namespace audio {


    ////////////////////////////////
    ///     OGG FILE READING     ///
    ////////////////////////////////


    struct packet {
        unsigned char* data;
        size_t len;
    };

    class ogg_context : public no_copy, no_move {
    public:
        ogg_context(std::string filename);
        ~ogg_context();
        packet read_packet(); // Read a packet from the ogg stream
        bool eof() { return file.gcount() == 0; }
    private:
        void read_page(); // Read an ogg page from disk into memory
        std::ifstream file;
        ogg_sync_state state;
        ogg_stream_state streamstate;
        int _unread_packets = 0;
    };

    ogg_context::ogg_context(std::string filename) : file(filename, std::ios::in | std::ios::binary) {
        if (!file.is_open()) {
            throw std::runtime_error("File not found");
        }
        int ret = ogg_sync_init(&state);
        assert(ret == 0);
        // Requesting a packet before reading a page in causes a segfault.
        // The first page contains the header, which is ignored by the rest of this code.
        read_page();
    }

    ogg_context::~ogg_context() {
        ogg_sync_clear(&state);
        ogg_stream_clear(&streamstate);
    }

    void ogg_context::read_page() {
        ogg_page page;
        int total_bytes = 0;
        while(ogg_sync_pageout(&state, &page) != 1) {
            char* buffer = ogg_sync_buffer(&state, 4096);
            file.read(buffer, 4096);;
            int bytes = file.gcount();
            total_bytes += bytes;
            ogg_sync_wrote(&state, bytes);
            if (bytes == 0) {
                break;
            }
        }

        if (total_bytes == 0)
            return;

        if (ogg_page_bos(&page)) {
            int serial = ogg_page_serialno(&page);
            ogg_stream_init(&streamstate, serial);
        }

        int ret = ogg_stream_pagein(&streamstate, &page);
        assert(ret == 0);
        _unread_packets += ogg_page_packets(&page);
    }

    packet ogg_context::read_packet() {
        ogg_packet p;
        int ret = ogg_stream_packetout(&streamstate, &p);
        while (ret == 0) {
            read_page();
            ret = ogg_stream_packetout(&streamstate, &p);
            if (eof()) break;
        }
        if (ret != 1)  return packet{nullptr, 0};
        return packet{p.packet, p.bytes};
    }


    ///////////////////////////////////////////
    ///     AUDIO ENGINE IMPLEMENTATION     ///
    ///////////////////////////////////////////


    void engine::initialize(int sample_rate, output_mode num_channels) {
        _sample_rate = sample_rate;
        _num_channels = int(num_channels);
        SDL_InitSubSystem(SDL_INIT_AUDIO);
        SDL_AudioSpec want;
        SDL_zero(want);
        want.freq = sample_rate;
        want.format = AUDIO_S16;
        want.channels = int(num_channels);

        _device_id = SDL_OpenAudioDevice(NULL, 0, &want, &want, SDL_AUDIO_ALLOW_ANY_CHANGE);
        SDL_PauseAudioDevice(_device_id, 0);
    };

    void engine::load_from_file(std::string filename, sink_channel channel) {
        // No point abstracting over the opus decoder, as it's only used here
        int error = 0;
        OpusDecoder* decoder = opus_decoder_create(_sample_rate, int(_num_channels), &error);
        assert(error == OPUS_OK);
        ogg_context context(filename);

        std::vector<std::vector<sample>> packets;
        size_t total_samples = 0;
        do {
            packet p = context.read_packet();
            // Missing packet, or Header Packet
            if(p.data == nullptr || (p.data[0] == 'O' && p.data[1] == 'p')) {
                continue;
            }

            int num_samples = opus_packet_get_nb_samples(p.data, p.len, _sample_rate);
            if (num_samples == OPUS_BAD_ARG || num_samples == OPUS_INVALID_PACKET) {
                printf("invalid audio packet, skipping\n");
                continue;
            }

            packets.emplace_back(std::vector<sample>(num_samples * int(_num_channels)));
            int read_samples = opus_decode(decoder, p.data, p.len, packets.back().data(), num_samples, 0);
            total_samples += read_samples;
        } while (!context.eof());

        opus_decoder_destroy(decoder);

        tracks.emplace_back();
        auto& track = tracks.back();
        track.channel = channel;
        track.data = std::vector<sample>(total_samples * int(_num_channels));
        size_t dest_offset = 0;
        for (auto& packet : packets) {
            memcpy(track.data.data() + dest_offset, packet.data(), packet.size() * sizeof(sample));
            dest_offset += packet.size();
        }
    }

    stream_id engine::play_track(size_t track_id) {
        assertion(track_id < tracks.size(), "Invalid track ID");
        auto& track_buffer = tracks[track_id];
        stream_id id = _active_tracks.first_free_id();
        _active_tracks.insert(id, stream(track_buffer.data.data(), track_buffer.data.size(), track_buffer.channel));
        return id;
    }

    void engine::mix_audio(size_t num_samples) {
        std::vector<sample> buffer = std::vector<sample>(num_samples * int(_num_channels));
        for (size_t i = 0; i < buffer.size(); i++) {
            float accumulator = 0;
            float gain = 1.0f / _active_tracks.size();
            for (auto it = _active_tracks.begin(); it != _active_tracks.end(); ++it) {
                auto& stream = *it;
                accumulator += float(stream.get_sample()) * _sink_volumes[(int)(stream.channel())];
                if (stream.eos()) {
                    _active_tracks.remove(it.index());
                }
            }
            buffer[i] = sample(accumulator * gain) * _sink_volumes[(int)sink_channel::master];
        }
        SDL_QueueAudio(_device_id, buffer.data(), buffer.size() * sizeof(sample));
    }

    void engine::stop_stream(stream_id track) { _active_tracks.remove(track); }
    void engine::set_channel_volume(sink_channel channel, float volume) { _sink_volumes[int(channel)] = (volume / 100.0); }

    engine::stream::stream(const sample* data, size_t size, sink_channel channel) : _data(data), _buffer_size(size), _channel(channel) {}
    bool engine::stream::eos() const { return _tracker >= _buffer_size; }
    sink_channel engine::stream::channel() { return _channel; }
    sample engine::stream::get_sample() {
        sample s = _data[_tracker];
        _tracker++;
        return s;
    }
}
