    #include "avs/registry.hpp"
    #include "avs/core.hpp"
    #include <vector>
    #include <cstdio>
    #include <filesystem>
    #include <cmath>
    #include <cstring>

    using namespace avs;

    static void write_ppm(const char* path, const FrameBufferView& fb){
      FILE* f = std::fopen(path, "wb");
      if(!f){ std::perror("fopen"); return; }
      std::fprintf(f, "P6
%d %d
255
", fb.width, fb.height);
      // strip alpha
      for(int y=0;y<fb.height;y++){
        const uint8_t* row = fb.data + y * fb.stride;
        for(int x=0;x<fb.width;x++){
          std::fputc(row[x*4+0], f);
          std::fputc(row[x*4+1], f);
          std::fputc(row[x*4+2], f);
        }
      }
      std::fclose(f);
    }

    static void gen_audio(AudioFeatures& af, int frame_idx, int sr, int nsamps, float freq){
      af.oscL.resize(nsamps);
      af.oscR.resize(nsamps);
      double t0 = frame_idx * (double)nsamps / (double)sr;
      for(int i=0;i<nsamps;i++){
        double t = t0 + i / (double)sr;
        float s = std::sin(2*M_PI*freq*t);
        af.oscL[i] = s;
        af.oscR[i] = s;
      }
      af.beat = ((frame_idx % 30) == 0);
      af.sample_rate = sr;
    }

    int main(){
      const int W=640, H=480;
      std::vector<uint8_t> buf_cur(W*H*4, 0), buf_prev(W*H*4, 0);
      FrameBufferView fb_cur{buf_cur.data(), W, H, W*4};
      FrameBufferView fb_prev{buf_prev.data(), W, H, W*4};
      FrameBuffers fbs{fb_cur, fb_prev};

      // Register effects
      Registry reg;
      register_builtin_effects(reg);

      // Build a tiny pipeline: Clear -> Starfield -> Oscilloscope -> Fadeout
      auto star = reg.create("starfield");
      auto osc  = reg.create("oscilloscope");
      auto fade = reg.create("fadeout");
      auto clear= std::make_unique<avs::ClearScreenEffect>();

      InitContext ictx{{W,H},{true,false,false,true},true,60};
      clear->init(ictx); star->init(ictx); osc->init(ictx); fade->init(ictx);

      // Timing
      TimingInfo tm; tm.deterministic = true; tm.fps_hint = 60;

      // Render 120 frames
      std::filesystem::create_directories("out");
      const int frames = 120;
      const int sr = 44100;
      const int hop = 1024;

      for(int fi=0; fi<frames; ++fi){
        tm.frame_index = fi;
        tm.t_seconds = fi * (double)hop / (double)sr;
        tm.dt_seconds = (double)hop / (double)sr;

        // swap buffers: previous <- current
        std::memcpy(buf_prev.data(), buf_cur.data(), buf_cur.size());

        // construct process context
        AudioFeatures af;
        gen_audio(af, fi, sr, hop, 220.0f);

        ProcessContext pctx{tm, af, fbs, nullptr, nullptr};

        // Clear, then draw effects into current
        clear->process(pctx, fbs.current);
        star->process(pctx, fbs.current);
        osc->process(pctx, fbs.current);
        fade->process(pctx, fbs.current);

        char path[256];
        std::snprintf(path, sizeof(path), "out/frame_%04d.ppm", fi);
        write_ppm(path, fbs.current);
      }

      return 0;
    }
