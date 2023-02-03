#ifndef AUDIO_H
#define AUDIO_H

#include <common/marked_array.h>

#include <cstdint>
#include <vector>
#include <string>

namespace audio {
    using sample = int16_t;
    using stream_id = uint8_t;

    enum class sink_channel {
        music,
        effects,
        master
    };



    class engine {
    public:
        enum class output_mode {
            mono = 1,
            stereo = 2
        };

        void initialize(int sample_rate, output_mode nc);
        void load_from_file(std::string filename, sink_channel channel);
        void mix_audio(size_t num_samples);
        stream_id play_track(size_t track_id);
        void stop_stream(stream_id track);
        void set_channel_volume(sink_channel channel, float volume);
    private:
        class track {
        public:
            std::vector<sample> data;
            sink_channel channel;
        };

        class stream {
        public:
            stream(const sample* data, size_t buffer_size, sink_channel channel);
            stream() = default;
            sample get_sample();
            bool eos() const;
            sink_channel channel();
        private:
            const sample* _data = nullptr;
            sink_channel _channel;
            size_t _buffer_size = 0;
            size_t _tracker = 0;
        };

        marked_array<stream, 32> _active_tracks;
        std::vector<track> tracks;
        std::array<float, 3> _sink_volumes;
        int _sample_rate;
        uint8_t _num_channels;
        int _device_id = 0;
    };
}
#endif //AUDIO_H
