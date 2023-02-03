#ifndef ENGINE_H
#define ENGINE_H

#include "renderer.h"
#include "audio.h"
#include "ecs.h"

class engine {
public:
    engine() {
        audio.initialize(48000, audio::engine::output_mode::stereo);
        audio.load_from_file("440.opus", audio::sink_channel::effects);
        audio.set_channel_volume(audio::sink_channel::master, 100);
        audio.set_channel_volume(audio::sink_channel::effects, 100);
    }
    void send_command(int argc, char** argv);
    renderer r;
    ecs::entity_manager ee;
private:
    audio::engine audio;
};

#endif //ENGINE_H
