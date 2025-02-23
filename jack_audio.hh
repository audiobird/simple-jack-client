#pragma once

#include <array>
#include <functional>
#include <iostream>
#include <jack/jack.h>
#include <span>

template <unsigned num_inputs, unsigned num_outputs>
  requires(num_inputs > 0 || num_outputs > 0)
class Jack {
  jack_client_t *client;
  std::array<jack_port_t *, num_inputs> in;
  std::array<jack_port_t *, num_outputs> out;

  using InputChannelBuf = std::span<const float>;
  using OutputChannelBuf = std::span<float>;

public:
  class Context {
    using Inputs = std::array<InputChannelBuf, num_inputs>;
    using Outputs = std::array<OutputChannelBuf, num_outputs>;

  public:
    Inputs in;
    Outputs out;
    unsigned get_block_size() const {
      if constexpr (num_outputs) {
        return out[0].size();
      } else {
        return in[0].size();
      }
    }
  };

  using ProcessCallback = std::move_only_function<void(Context)>;

  Jack(std::string_view client_name, ProcessCallback &&process_cb)
      : process{std::move(process_cb)} {

    client =
        jack_client_open(client_name.data(), JackNullOption, nullptr, nullptr);
    if (!client) {
      std::cout << "failed to open jack client" << std::endl;
      return;
    }

    auto register_ports = [this](std::span<jack_port_t *> port,
                                 JackPortFlags flags, std::string prefix) {
      for (unsigned int i = 0; i < port.size(); i++) {
        const auto name = prefix + std::to_string(i);
        port[i] = jack_port_register(client, name.c_str(),
                                     JACK_DEFAULT_AUDIO_TYPE, flags, 0);
        if (!port[i]) {
          std::cout << "failed to register jack port \"" + name + "\""
                    << std::endl;
          return;
        }
      }
    };

    register_ports(in, JackPortIsInput, "in_");
    register_ports(out, JackPortIsOutput, "out_");

    if (jack_set_process_callback(client, onJackProcess, this)) {
      std::cout << "failed to set jack callback" << std::endl;
      return;
    }
  }

  ~Jack() { jack_client_close(client); }

  bool start() {
    if (jack_activate(client)) {
      jack_client_close(client);
      return false;
    }
    return true;
  }

  bool stop() {
    if (jack_deactivate(client)) {
      jack_client_close(client);
      return false;
    }
    return true;
  }

private:
  ProcessCallback process;

  void onJackProcess(jack_nframes_t nFrames) {
    std::array<InputChannelBuf, num_inputs> buff_in{};
    std::array<OutputChannelBuf, num_outputs> buff_out{};

    auto get_ports = [nFrames](auto buff, std::span<jack_port_t *> port) {
      for (auto i = 0u; i < buff.size(); i++) {
        buff[i] = {
            reinterpret_cast<float *>(jack_port_get_buffer(port[i], nFrames)),
            nFrames};
      }
    };

    get_ports(std::span{buff_in}, in);
    get_ports(std::span{buff_out}, out);

    process({buff_in, buff_out});
  }

  static int onJackProcess(jack_nframes_t nFrames, void *args) {
    static_cast<Jack *>(args)->onJackProcess(nFrames);
    return 0;
  }
};
