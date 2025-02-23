# Simple Header Only C++ Jack Client

An extremely quick and easy way to get audio in or out of your c++ program 

Example:

```
#include "sound/jack_audio.hh"
#include <iostream>

constexpr auto num_inputs = 2;
constexpr auto num_outputs = 2;
using Audio = Jack<num_inputs, num_outputs>;

int main() {
  // simply copy inputs to outputs
  auto audio_callback = [](Audio::Context ctx) {
    for (auto i = 0u; i < ctx.get_block_size(); ++i) {
      ctx.out[0][i] = ctx.in[0][i];
      ctx.out[1][i] = ctx.in[1][i];
    }
  };

  // register jack client and callback
  Audio audio{"PD", audio_callback};

  // start processing audio
  if (!audio.start()) {
    std::cout << "failed to start audio" << std::endl;
    return 1;
  }

  while (true) {
  }

  if (!audio.stop()) {
    std::cout << "failed to stop audio" << std::endl;
    return 1;
  }
}

```

