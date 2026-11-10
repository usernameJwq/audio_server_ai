# sherpa-onnx TTS C++ 封装调研

> 调研时间：2026-11-10　|　代码基线：sherpa-onnx `master` / 最新 release **v2.0.1**（2026-10-26）
> 目标：用 C++ 封装 sherpa-onnx 的离线 TTS，明确「支持哪些模型 / 用哪个 demo / 用哪个 release 包 / 现在效果最好的 TTS 是谁」。

---

## 0. 结论先行（TL;DR）

| 问题 | 结论 |
|---|---|
| 用什么 C++ 接口 | `sherpa-onnx/c-api/cxx-api.h` 中的 `sherpa_onnx::cxx::OfflineTts`（RAII，header-only 封装 C API） |
| 中文+英文首选模型 | **Kokoro multi-lang v1.1-zh**（103 音色，24 kHz，311 MB） |
| 轻量/低延迟中文 | **Matcha icefall-zh-baker**（1 女声，22.05 kHz，73 MB，RPi4 4 线程 RTF 0.39） |
| 中英双语稳定老模型 | **MeloTTS**（`vits-melo-tts-zh_en`，1 音色，163 MB）或 **Piper zh_CN**（chaowen / xiao_ya） |
| 零样本音色克隆 | **ZipVoice**（中英，需参考音频+参考文本）或 **PocketTTS**（英文，只需参考音频） |
| 英文超轻量 | **KittenTTS** / **Piper en_US** / **Supertonic 3** |
| C++ demo | `cxx-api-examples/` 下 8 个 TTS 示例；最小可读的是 `offline-tts-piper-cxx-api.cc`，中英对照看 `kokoro-tts-zh-en-cxx-api.cc` |
| Windows release 包 | `sherpa-onnx-v2.0.1-win-x64-shared-MD-Release.tar.bz2`（含 exe+lib+include）；只取库用 `...-shared-MD-Release-lib.tar.bz2` |
| 模型在哪 | 单独挂在 release tag **`tts-models`**，不在 v2.0.1 资产里 |
| 当前最强 TTS | 闭源：ElevenLabs **Eleven v4**（AA Elo 1338）；中文 API：**Qwen-Audio-3.1-TTS-Plus**（1299）；开源权重：**Fish Audio Drama 3**（1271） |
| sherpa-onnx 在榜上的位置 | 它支持的模型里只有 **Kokoro 82M** 进榜：AA 开源榜第 10（1062），TTS Arena 第 24（1469） |

**一句话**：sherpa-onnx 的强项是「离线、跨平台、C++ 可嵌入、CPU 可跑」，不是「音质 SOTA」。要最高自然度就用云 API 或换 CosyVoice3 / IndexTTS2 / Fish S2 这类框架；要在端上稳定跑中英双语，Kokoro v1.1 + C++ 封装是最优解。

---

## 1. sherpa-onnx 的 TTS 能力边界

- 全部是 **非流式（offline）TTS**：输入整段文本，输出整段 PCM；没有 token 级流式合成。
- 支持 **CPU**（默认）和 ONNX Runtime 的 CUDA / DirectML 等 provider，但 TTS 模型普遍很小，CPU 足够。
- 官方支持平台：Windows x64/x86/ARM64、Linux x64/aarch64/arm/riscv64、macOS、Android、iOS、HarmonyOS、WASM。
- Windows 只支持 **MSVC / Visual Studio 2022**，**不支持 MinGW**。
- 模型全部转换为 ONNX，运行时只依赖 onnxruntime，不依赖 PyTorch。

---

## 2. 支持的 TTS 模型家族

### 2.1 总表（`OfflineTtsModelConfig` 的 7 个分支）

| 家族 | C++ 配置字段 | 语言 | 多音色 | 零样本克隆 | 备注 |
|---|---|---|---|---|---|
| **VITS** | `config.model.vits` | 多语言（Piper 生态）+ MeloTTS | 支持（模型相关） | 否 | 最成熟、模型最多；Piper 也属于 VITS |
| **Matcha** | `config.model.matcha` | 中、英、中英 | 否（单说话人） | 否 | 需要外挂 vocoder（vocos） |
| **Kokoro** | `config.model.kokoro` | 中、英、中英、多语 | 是 | 否 | 82M 参数，质量/体积比最好 |
| **KittenTTS** | `config.model.kitten` | 英文 | 是 | 否 | 超小模型，nano/micro/mini 多档 |
| **ZipVoice** | `config.model.zipvoice` | 中英 | 克隆音色 | **是**（音频+文本） | 需要 `reference_audio` + `reference_text` |
| **PocketTTS** | `config.model.pocket` | 英文 | 克隆音色 | **是**（只要音频） | 不需要参考文本 |
| **Supertonic 3** | `config.model.supertonic` | 31 种语言（**不含中文**） | 是 | 否 | int8 多文件，含 `tts.json` / `unicode_indexer` |

> 另有 **MMS**（Meta 多语言 VITS）通过 VITS 分支加载，主要用于低资源语言；**Piper** 的所有 voice 也走 VITS 分支。

### 2.2 中文 / 中英模型清单（重点）

| 模型 | 语言 | 音色数 | 采样率 | 体积 | 需要的文件 | 适合场景 |
|---|---|---|---|---|---|---|
| `kokoro-multi-lang-v1_1` | 中+英 | **103**（中文女 3–57，男 58–102） | 24 kHz | 311 MB | `model.onnx` `voices.bin` `tokens.txt` `lexicon-us-en.txt` `lexicon-zh.txt` | 默认首选，音色丰富 |
| `kokoro-multi-lang-v1_0` | 中+英 | 54（中文 45–52） | 24 kHz | ~330 MB | 同上 | 老版，兼容性更好 |
| `matcha-icefall-zh-baker` | 中文 | 1 女声 | 22.05 kHz | 73 MB + 51 MB vocoder | `model-steps-3.onnx` `lexicon.txt` `tokens.txt` + `vocos-22khz-univ.onnx` + FST | 低延迟、CPU 友好 |
| `matcha-icefall-zh-en` | 中+英 | 1 | 16 kHz | 72 MB + 51 MB vocoder | `model-steps-3.onnx` + `vocos-16khz-univ.onnx` + lexicon/tokens | 中英混读、轻量 |
| `vits-melo-tts-zh_en` | 中+英 | 1 | 44.1 kHz（以 `SampleRate()` 为准） | 163 MB | `model.onnx` `lexicon.txt` `tokens.txt` + FST | 老牌稳定，英文受 lexicon 限制 |
| `vits-piper-zh_CN-chaowen-medium` | 中文 | 1 女声 | 22.05 kHz | 约 60 MB | `.onnx` `tokens.txt` `lexicon.txt` + FST | 体积小、部署简单 |
| `vits-piper-zh_CN-xiao_ya-medium` | 中文 | 1 女声 | 22.05 kHz | 约 60 MB | 同上 | 同上 |
| `sherpa-onnx-zipvoice-distill-int8-zh-en-emilia` | 中+英 | 克隆 | 24 kHz | int8，较小 | `encoder.int8.onnx` `decoder.int8.onnx` `lexicon.txt` `tokens.txt` + `vocos_24khz.onnx` | 定制音色/克隆 |

**选型要点**
- 要「多个中文音色 + 中英混读」→ Kokoro v1.1。
- 要「最快、最稳、单音色」→ Matcha zh-baker（RTF 0.39 @ RPi4 4 线程）。
- 要「44.1 kHz 高采样率」→ MeloTTS。
- 要「指定人声」→ ZipVoice，但必须提供**逐字准确**的参考文本，否则音质明显下降。
- Supertonic 3 虽然支持 31 语言，但**没有中文**，中文项目不要选。

### 2.3 各家族的 C++ 配置字段（来自 `cxx-api.h`）

```cpp
// VITS
config.model.vits.model;      // xxx.onnx
config.model.vits.lexicon;    // lexicon.txt（可空）
config.model.vits.tokens;     // tokens.txt
config.model.vits.length_scale = 1.0f;

// Matcha
config.model.matcha.acoustic_model; // model-steps-3.onnx
config.model.matcha.vocoder;        // vocos-*.onnx
config.model.matcha.lexicon;
config.model.matcha.tokens;

// Kokoro
config.model.kokoro.model;    // model.onnx
config.model.kokoro.voices;   // voices.bin
config.model.kokoro.tokens;
config.model.kokoro.lexicon;  // "lexicon-us-en.txt,lexicon-zh.txt"
config.model.kokoro.lang;     // 可选

// ZipVoice（克隆）
config.model.zipvoice.encoder;
config.model.zipvoice.decoder;
config.model.zipvoice.vocoder;
config.model.zipvoice.tokens;
config.model.zipvoice.lexicon;
// 生成时再传 reference_audio / reference_text

// Pocket（克隆）
config.model.pocket.lm_flow;
config.model.pocket.lm_main;
config.model.pocket.encoder;
config.model.pocket.decoder;
config.model.pocket.text_conditioner;
config.model.pocket.vocab_json;
config.model.pocket.token_scores_json;

// Supertonic
config.model.supertonic.duration_predictor;
config.model.supertonic.text_encoder;
config.model.supertonic.vector_estimator;
config.model.supertonic.vocoder;
config.model.supertonic.tts_json;
config.model.supertonic.unicode_indexer;
config.model.supertonic.voice_style;
```

> 从 v2.0.0 起，`data_dir` 被标记为 deprecated（不再使用 espeak-ng-data），需要自行预分词时改传 `GenerationConfig::phoneme_codepoints` 或 `::tokens`。

---

## 3. C++ API 与封装

### 3.1 头文件与库

| 项目 | 值 |
|---|---|
| 头文件 | `#include "sherpa-onnx/c-api/cxx-api.h"` |
| 命名空间 | `sherpa_onnx::cxx` |
| 核心类 | `OfflineTts`（MoveOnly，RAII） |
| 依赖库 | `sherpa-onnx-cxx-api` → `sherpa-onnx-c-api` → `onnxruntime` |
| 音频输出 | `GeneratedAudio{ std::vector<float> samples; int32_t sample_rate; }`，样本范围 `[-1, 1]` |
| 写 WAV | `sherpa_onnx::cxx::WriteWave(filename, {samples, sample_rate})` |

> `cxx-api.h` 是纯头文件封装，真正的 ABI 在 `sherpa-onnx-c-api` 里。因此即使预编译包里没带 `cxx-api.h`，也可以从**同版本源码**复制这一个头文件使用。

### 3.2 核心 API

```cpp
class OfflineTts {
 public:
  static OfflineTts Create(const OfflineTtsConfig &config);

  int32_t SampleRate() const;     // 输出采样率
  int32_t NumSpeakers() const;    // 音色数（单说话人可能返回 0 或 1）
  std::string Lang() const;       // 例如 "en-us"，可能为空

  // 简单接口
  GeneratedAudio Generate(const std::string &text, int32_t sid = 0,
                          float speed = 1.0,
                          OfflineTtsCallback callback = nullptr,
                          void *arg = nullptr) const;

  // 高级接口（推荐）
  GeneratedAudio Generate(const std::string &text,
                          const GenerationConfig &config,
                          OfflineTtsCallback callback = nullptr,
                          void *arg = nullptr) const;

  // 返回 shared_ptr 的版本
  std::shared_ptr<GeneratedAudio> Generate2(...);
};
```

`GenerationConfig` 关键字段：

| 字段 | 作用 |
|---|---|
| `sid` | 音色 ID（Kokoro v1.1：中文女 3–57，男 58–102） |
| `speed` | 语速，>1 更快（部分模型支持） |
| `silence_scale` | 句间静音缩放，默认 0.2 |
| `num_steps` | flow matching 步数（ZipVoice/Pocket/Supertonic），质量/速度权衡 |
| `reference_audio` / `reference_sample_rate` | 克隆用参考音频（mono float） |
| `reference_text` | ZipVoice 必填，必须与参考音频逐字一致 |
| `phoneme_codepoints` / `tokens` | 自行预分词时使用，二者互斥 |
| `extra` | 模型特定参数（字符串 map） |

回调签名：

```cpp
int32_t (*)(const float *samples, int32_t num_samples,
            float progress, void *arg);
// 返回 1 继续，返回 0 提前停止
// samples 指针只在回调期间有效，需要保存请自行拷贝
```

### 3.3 最小可运行示例（Kokoro 中英）

```cpp
#include <cstdio>
#include <string>
#include "sherpa-onnx/c-api/cxx-api.h"

using namespace sherpa_onnx::cxx;

int main() {
  OfflineTtsConfig config;
  const std::string dir = "./kokoro-multi-lang-v1_1";
  config.model.kokoro.model   = dir + "/model.onnx";
  config.model.kokoro.voices  = dir + "/voices.bin";
  config.model.kokoro.tokens  = dir + "/tokens.txt";
  config.model.kokoro.lexicon = dir + "/lexicon-us-en.txt," + dir + "/lexicon-zh.txt";
  config.model.num_threads = 2;
  config.model.debug = false;

  auto tts = OfflineTts::Create(config);

  GenerationConfig gen;
  gen.sid = 3;          // zf_001，中文女声
  gen.speed = 1.0f;
  gen.silence_scale = 0.2f;

  GeneratedAudio audio = tts.Generate(
      "中英文语音合成测试。This is generated by sherpa-onnx.", gen);

  WriteWave("./out.wav", {audio.samples, audio.sample_rate});
  printf("sample_rate=%d, samples=%zu\n", audio.sample_rate, audio.samples.size());
  return 0;
}
```

### 3.4 封装设计建议

1. **RAII 持有**：`OfflineTts` 是 MoveOnly 且不可默认构造，建议：
   ```cpp
   class TtsEngine {
    public:
     explicit TtsEngine(const OfflineTtsConfig &cfg)
         : tts_(std::make_unique<OfflineTts>(OfflineTts::Create(cfg))) {}
     GeneratedAudio Speak(const std::string &text, const GenerationConfig &g) {
       std::lock_guard<std::mutex> lock(mu_);
       return tts_->Generate(text, g);
     }
    private:
     std::unique_ptr<OfflineTts> tts_;
     std::mutex mu_;
   };
   ```
2. **模型常驻**：Create 只做一次（加载 70–330 MB 模型），不要每句话 Create。
3. **多线程**：官方未承诺单实例线程安全；保守做法是「每线程一个 `OfflineTts` 实例」或「单实例 + mutex」。实例之间可共享模型文件，但内存不共享。
4. **长文本**：`OfflineTtsConfig::max_num_sentences` 控制一次处理的句子数（默认 1，避免 OOM）；配合回调可做到「句级流式」播报。
5. **音色映射**：Kokoro 的 sid 与名字对应关系见官方文档（v1.1：0 `af_maple`、1 `af_sol`、2 `bf_vale`、3–57 中文女、58–102 中文男）。
6. **采样率适配**：Playback/重采样要以 `audio.sample_rate` 为准，不要硬编码 16k/24k。
7. **异常与失败**：Create 失败时要做兜底（返回空/抛异常取决于版本），封装层应统一转成 `std::optional` 或错误码。
8. **文本预处理**：中文数字、日期、电话号码用 `rule_fsts`（模型自带的 `date.fst` / `number.fst` / `phone.fst`）：
   ```cpp
   config.rule_fsts = dir + "/date.fst," + dir + "/number.fst";
   ```

### 3.5 构建

**方式 A：源码树内构建（官方 demo 的方式）**

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DSHERPA_ONNX_ENABLE_TTS=ON \
  -DSHERPA_ONNX_ENABLE_C_API=ON \
  -DBUILD_SHARED_LIBS=ON
cmake --build build -j
# 产物：build/bin/sherpa-onnx-offline-tts(.exe)
```

**方式 B：使用预编译包（推荐给业务工程）**

`CMakeLists.txt` 示例（Windows，MSVC）：

```cmake
cmake_minimum_required(VERSION 3.13)
project(tts_demo CXX)
set(CMAKE_CXX_STANDARD 14)

set(SHERPA_ROOT "C:/sdk/sherpa-onnx-v2.0.1-win-x64-shared-MD-Release")

add_executable(tts_demo main.cc)
target_include_directories(tts_demo PRIVATE ${SHERPA_ROOT}/include)
target_link_directories(tts_demo PRIVATE ${SHERPA_ROOT}/lib)
target_link_libraries(tts_demo PRIVATE
  sherpa-onnx-cxx-api
  sherpa-onnx-c-api
  onnxruntime)

# 运行时把 dll 拷到 exe 同目录
add_custom_command(TARGET tts_demo POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${SHERPA_ROOT}/lib/onnxruntime.dll
          ${SHERPA_ROOT}/lib/sherpa-onnx-c-api.dll
          ${SHERPA_ROOT}/lib/sherpa-onnx-cxx-api.dll
          $<TARGET_FILE_DIR:tts_demo>)
```

Linux/macOS 也可以用 pkg-config：

```bash
export PKG_CONFIG_PATH=/path/to/sherpa-onnx-shared:$PKG_CONFIG_PATH
g++ -std=c++14 main.cc -o tts_demo \
  $(pkg-config --cflags --libs sherpa-onnx)
```

> 静态链接时库顺序敏感，官方提供 `sherpa-onnx-static.pc` / `sherpa-onnx-static-no-tts.pc`，建议直接用 pkg-config，不要手写 `-l` 顺序。

---

## 4. Demo 对照表（`cxx-api-examples/`）

| 文件 | 模型 | 说明 |
|---|---|---|
| `offline-tts-piper-cxx-api.cc` | Piper VITS | **最小示例**，只有 model/tokens/lexicon |
| `kokoro-tts-en-cxx-api.cc` | Kokoro 英文 | 单语英文 |
| `kokoro-tts-zh-en-cxx-api.cc` | Kokoro 中英 | sid=50、lexicon 两个文件 |
| `matcha-tts-zh-cxx-api.cc` | Matcha zh-baker | 中文 + vocoder |
| `matcha-tts-en-cxx-api.cc` | Matcha en | 英文 + vocoder |
| `kitten-tts-en-cxx-api.cc` | KittenTTS | 英文小模型 |
| `pocket-tts-en-cxx-api.cc` | PocketTTS | 零样本克隆（只要参考音频） |
| `supertonic-tts-en-cxx-api.cc` | Supertonic 3 | 多语言、多文件 |
| `zipvoice-tts-zh-en-cxx-api.cc` | ZipVoice | 零样本克隆（音频+文本） |

通用命令行 demo：

```bash
./bin/sherpa-onnx-offline-tts \
  --kokoro-model=./kokoro-multi-lang-v1_1/model.onnx \
  --kokoro-voices=./kokoro-multi-lang-v1_1/voices.bin \
  --kokoro-tokens=./kokoro-multi-lang-v1_1/tokens.txt \
  --kokoro-lexicon=./kokoro-multi-lang-v1_1/lexicon-us-en.txt,./kokoro-multi-lang-v1_1/lexicon-zh.txt \
  --sid=3 --speed=1.0 \
  --output-filename=./out.wav \
  "你好，这是一段中英文测试。Hello from sherpa-onnx."
```

---

## 5. Release 包选择（v2.0.1）

### 5.1 先选平台和链接方式

| 场景 | 包 |
|---|---|
| **Windows x64 + VS2022（推荐）** | `sherpa-onnx-v2.0.1-win-x64-shared-MD-Release.tar.bz2` |
| 只拿库和头文件（不含 exe） | `sherpa-onnx-v2.0.1-win-x64-shared-MD-Release-lib.tar.bz2` |
| 希望 exe 不依赖 VC++ 运行时 | `...-static-MT-Release.tar.bz2` |
| Linux x64 | `sherpa-onnx-v2.0.1-linux-x64-shared.tar.bz2`（库：`-shared-lib`） |
| macOS ARM | `sherpa-onnx-v2.0.1-osx-arm64-shared.tar.bz2` |
| Android | `sherpa-onnx-v2.0.1-android.tar.bz2` |
| CUDA | `...-cuda-12.x-...-win-x64-cuda.tar.bz2` |

### 5.2 命名规则

```
sherpa-onnx-v2.0.1-win-x64-shared-MD-Release[-no-tts][-lib].tar.bz2
                  │       │      │   │      │        │      │
                  │       │      │   │      │        │      └─ 只含库/头文件
                  │       │      │   │      │        └──────── 不含 TTS（体积小，别选错）
                  │       │      │   │      └───────────────── Release / Debug / MinSizeRel / RelWithDebInfo
                  │       │      │   └──────────────────────── MD=/MD 动态 CRT；MT=/MT 静态 CRT
                  │       │      └──────────────────────────── shared / static
                  │       └─────────────────────────────────── win-x64 / win-arm64 / linux-x64 / osx-arm64 ...
                  └─────────────────────────────────────────── 版本号
```

- **MD vs MT**：MD 依赖 VC++ Redistributable（`vcruntime140.dll` 等），MT 把 CRT 静态链接进 exe。业务安装包一般选 **MD**；绿色单文件/受限环境选 **MT**。
- **shared vs static**：shared 生成 `.dll` + `.lib`，便于替换升级；static 只生成 `.a/.lib`，体积大但部署简单。
- **千万不要选带 `-no-tts` 的包**——TTS 代码根本没编进去。
- Windows 包用 **Visual Studio 2022** 构建，MSVC 工具集要匹配；MinGW 不可用。

### 5.3 包内结构（以官方 install 树为准）

```
sherpa-onnx-v2.0.1-win-x64-shared-MD-Release/
├── bin/
│   ├── sherpa-onnx-offline-tts.exe
│   ├── sherpa-onnx-offline-tts-play.exe
│   └── ...（ASR/VAD 等其它示例）
├── include/
│   └── sherpa-onnx/c-api/
│       ├── c-api.h
│       └── cxx-api.h        # 若包内缺失，从同版本源码复制
└── lib/
    ├── sherpa-onnx-c-api.lib / .dll
    ├── sherpa-onnx-cxx-api.lib / .dll
    ├── onnxruntime.lib / onnxruntime.dll
    └── sherpa-onnx.pc
```

> 说明：`-lib` 包只去掉 `bin/`，保留 `include/` + `lib/`。这一点官方文档没有逐字写明，**以实际解压结果为准**；C API 文档给出的 install 树是可靠的参考。

### 5.4 模型包不在 release 里

所有 TTS 模型都挂在单独的 tag：

```
https://github.com/k2-fsa/sherpa-onnx/releases/tag/tts-models
https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/<model>.tar.bz2
```

vocoder 单独下载：

```
https://github.com/k2-fsa/sherpa-onnx/releases/download/vocoder-models/vocos-22khz-univ.onnx
https://github.com/k2-fsa/sherpa-onnx/releases/download/vocoder-models/vocos-16khz-univ.onnx
https://github.com/k2-fsa/sherpa-onnx/releases/download/vocoder-models/vocos_24khz.onnx
```

---

## 6. 当前 TTS 排行榜（2026-11-10 实访）

### 6.1 先明确：TTS 没有统一的「准确率」榜

| 指标 | 含义 | 常见来源 |
|---|---|---|
| **Elo / Arena rating** | 人类盲听「哪个更像真人」的偏好分 | TTS Arena、Artificial Analysis |
| **WER / CER** | 合成语音再经 ASR 转写的字错率，衡量**可懂度** | Seed-TTS Eval、自测 |
| **SIM** | 与目标音色的说话人相似度 | Seed-TTS Eval、自测 |
| **RTF** | 实时率（生成 1 秒音频耗时 / 1 秒） | sherpa-onnx 官方 RTF 表 |

下面的榜单都是 **Elo（自然度偏好）**，不是字面意义的「准确率」。中文可懂度建议用 ASR 回测（见 7.5）。

### 6.2 Artificial Analysis — Provider Voice Arena

总榜（各厂商原生音色，前 10）：

| 排名 | 模型 | Elo | 样本数 | 发布 | 价格 |
|---|---|---|---|---|---|
| 1 | **ElevenLabs Eleven v4** | **1338 ±15** | 2,703 | 2026-09 | $80 /1M chars |
| 2 | ElevenLabs Eleven v4 Turbo | 1335 ±16 | 2,410 | 2026-09 | $40 /1M chars |
| 3 | **Qwen-Audio-3.1-TTS-Plus**（阿里） | **1299 ±14** | 2,450 | 2026-09 | $19.31 /1M chars |
| 4 | Cartesia Sonic 3.6 | 1295 ±13 | 3,297 | 2026-08 | $49 /1M chars |
| 5 | Speechify Simba 3.5（provisional） | 1294 ±17 | 1,674 | 2026-11 | $6.58 /1M chars |
| 6 | Google Gemini 3.8 Flash TTS | 1283 ±13 | 3,569 | 2026-09 | $16.49 /1M chars |
| 7 | Smallest.ai Lightning V3.1 Pro（provisional） | 1274 ±15 | 1,975 | 2026-10 | $19.50 /1M chars |
| 8 | **Fish Audio Drama 3**（开源权重） | 1271 ±17 | 1,672 | 2026-10 | $15 /1M chars |
| 9 | Qwen-Audio-3.0-TTS-Plus | 1267 ±13 | 2,961 | 2026-09 | $19.31 /1M chars |
| 10 | Inworld Realtime TTS-2 | 1267 ±14 | 2,674 | 2026-08 | $20.83 /1M chars |

**Open Weights 子榜**（对本地部署最有参考价值，前 10）：

| 排名 | 模型 | Elo |
|---|---|---|
| 1 | **Fish Audio Drama 3** | **1271** |
| 2 | Fish Audio Drama 3 Preview | 1239 |
| 3 | BreezeBlue Breeze TTS 2 | 1228 |
| 4 | **Ant Group Ming-3.0-tts** | **1185** |
| 5 | Gradium Phonon 31M (English) | 1114 |
| 6 | Fish Audio S2 Pro | 1106 |
| 7 | StepFun Step Audio EditX | 1098 |
| 8 | Mistral Voxtral TTS | 1092 |
| 9 | NVIDIA Magpie-Multilingual 357M | 1063 |
| 10 | **Kokoro 82M v1.0** | **1062** |

### 6.3 TTS Arena V2

共 41 个模型、12.6k 票（前 12）：

| 排名 | 模型 | Rating |
|---|---|---|
| 1 | Aurora | 1684 ±42 |
| 2 | star-june-2026（preliminary） | 1631 ±45 |
| 3 | Inworld TTS MAX | 1567 ±25 |
| 4 | CastleFlow v1.0（retired） | 1557 ±23 |
| 5 | Inworld TTS 1.5 MAX | 1565 ±37 |
| 6 | Hume Octave | 1551 ±27 |
| 7 | Papla P1（retired） | 1551 ±31 |
| 8 | Inworld TTS | 1543 ±24 |
| 9 | Gradium TTS（preliminary） | 1553 ±46 |
| 10 | **OpenAudio S2** | **1542 ±40** |
| 11 | MiniMax Speech 2.8 HD（preliminary） | 1545 ±48 |
| 12 | Smallest.ai Lightning v3.1 Pro（preliminary） | 1539 ±44 |

> 页面排序综合了置信区间：部分 `preliminary` 模型 rating 略高但样本少，排名会靠后；`retired` 表示已下架但保留历史分。

sherpa-onnx 相关模型的位置：

| 模型 | 排名 | Rating | 是否开源 |
|---|---|---|---|
| OpenAudio S2 | #10 | 1542 | 是（但 sherpa-onnx 不支持） |
| **Kokoro v1.0** | **#24** | **1469** | 是（sherpa-onnx 支持） |
| Chatterbox | #25 | 1460 | 是（sherpa-onnx 不支持） |
| OpenAudio S1 | #27 | 1465 | 是（sherpa-onnx 不支持） |
| Magpie Research Preview | #28 | 1456 | 是（sherpa-onnx 不支持） |

### 6.4 结论：排行榜 vs sherpa-onnx

1. **绝对第一梯队全是闭源 API**：Eleven v4、Qwen-Audio-3.1-TTS-Plus、Gemini 3.8 Flash TTS、Cartesia Sonic 3.6。要「最像真人」直接调 API，不要自己部署。
2. **开源权重第一梯队**：Fish Audio Drama 3（1271）、Breeze TTS 2（1228）、Ming-3.0-tts（1185）。这些模型**目前都不在 sherpa-onnx 的支持列表里**（Drama 3 / Ming 3.0 是 PyTorch 权重，需要自己转 ONNX + 写前端）。
3. **sherpa-onnx 可部署的最优解是 Kokoro 82M**：AA 开源榜第 10（1062），TTS Arena 第 24（1469）。它的定位是「82M 参数、CPU 实时、24 kHz」，不是「榜一」。
4. **中文场景别只看总榜**：AA / TTS Arena 以英文为主；中文音色和中文韵律要自己听、自己测 CER。
5. **ZipVoice / Matcha / Piper / Kitten / Supertonic 没有进入这些公开榜**，不代表差，而是「轻量离线模型」不在评测方的采样范围里；它们的优势是 RTF 和部署成本。

---

## 7. 中文场景怎么选（结合 RTF 与榜单）

### 7.1 官方 RTF（Raspberry Pi 4 Model B，4 线程）

| 模型 | 1 线程 | 2 线程 | 3 线程 | 4 线程 | 体积 |
|---|---|---|---|---|---|
| `vits-melo-tts-zh_en` | 6.727 | 3.877 | 2.914 | 2.518 | 163 MB |
| `matcha-icefall-zh-baker` | 0.892 | 0.536 | 0.432 | **0.391** | 73 MB |
| `kokoro-multi-lang-v1_1` | 7.635 | 4.470 | 3.430 | 3.191 | 311 MB |
| `kokoro-en-v0_19` | 6.629 | 3.870 | 2.999 | 2.774 | 330 MB |
| `vits-piper-en_US-lessac-medium` | 0.774 | 0.482 | 0.390 | 0.357 | 61 MB |
| `aishell3`（VITS，中文 174 音色） | 0.365 | 0.220 | 0.171 | 0.156 | 30 MB |

> x86 桌面 CPU 上会快 5–20 倍，Kokoro 在普通笔记本上通常也能实时。树莓派上 Kokoro 偏慢，选 Matcha/Piper。

### 7.2 推荐组合

| 需求 | 模型 | 理由 |
|---|---|---|
| 中英混读 + 多音色 + 质量优先 | **Kokoro v1.1** | 103 音色、24 kHz、开源榜最强可部署模型 |
| 中文 + 低延迟 + 嵌入式 | **Matcha zh-baker** | RTF 0.39，73 MB，单音色够用 |
| 中文 + 高采样率 + 稳定 | **MeloTTS** | 44.1 kHz，老牌，英文受 lexicon 限制 |
| 中文 + 极小体积 | **Piper zh_CN** | 约 60 MB，RTF 0.36 |
| 定制音色/克隆 | **ZipVoice** | 中英零样本，参考文本必须准确 |
| 纯英文 + 极小 | **KittenTTS nano/micro** | 几 MB 级，适合端侧 |
| 多语言（不含中文） | **Supertonic 3** | 31 语言，int8 |

### 7.3 中文可懂度自测（推荐）

公开榜单不测中文 CER，最可靠的办法是「TTS → ASR 回环」：

1. 准备 50–100 条中文测试文本（含数字、日期、多音字、中英混排）。
2. 用候选模型合成 wav。
3. 用 sherpa-onnx 的 **SenseVoice / Paraformer** 识别。
4. 算 CER：
   ```
   CER = (替换 + 删除 + 插入) / 参考字数
   ```
5. 同时记录 RTF、首句延迟、峰值内存。

这样得到的结论比任何公开 Elo 榜都更贴合你的业务。

---

## 8. 落地检查清单

- [ ] 确定语言：纯中文 / 中英混读 / 纯英文 / 多语言
- [ ] 确定音色：单音色够不够？需不需要克隆？
- [ ] 确定平台：Windows x64？Linux？ARM？内存上限？
- [ ] 选模型 → 下载 `tts-models` 包 → 核对文件列表
- [ ] 选 release 包：`win-x64-shared-MD-Release`（业务）/ `-lib`（只嵌入）
- [ ] 编译 demo：`cxx-api-examples/offline-tts-piper-cxx-api.cc` 跑通
- [ ] 换成目标模型，验证 `SampleRate()` / `NumSpeakers()` / `sid`
- [ ] 验证中文数字、日期、电话（`rule_fsts`）
- [ ] 压测 RTF、内存、并发（每线程一个实例）
- [ ] 用 ASR 回测 CER，确认可懂度
- [ ] 打包时带上 `onnxruntime.dll` / `sherpa-onnx-*.dll` 和模型目录

---

## 9. 未确认 / 需你复核的点

1. **`-lib` 包内部结构**：官方未逐字写明，本文按 C API install 树推断；建议下载后 `tar tjf` 核对是否包含 `include/sherpa-onnx/c-api/cxx-api.h`。若缺失，从同版本源码复制该头文件即可。
2. **MeloTTS 采样率**：官方模型页未标注，本文按 MeloTTS 原模型写 44.1 kHz；以运行时 `SampleRate()` 为准。
3. **Kokoro v1.1 文件清单**：本文按 v1.0 示例推断（`model.onnx` / `voices.bin` / `tokens.txt` / 两个 lexicon），下载后请 `ls` 核对。
4. **排行榜时效性**：数据为 2026-11-10 抓取；AA 榜单带 `provisional` 标记的模型仍在累积样本，排名会变。
5. **线程安全**：官方未承诺 `OfflineTts` 单实例并发安全，本文建议加锁或每线程独立实例。

---

## 10. 参考链接

**官方**
- 仓库：https://github.com/k2-fsa/sherpa-onnx
- TTS 文档：https://k2-fsa.github.io/sherpa/onnx/tts/index.html
- 全模型索引：https://k2-fsa.github.io/sherpa/onnx/tts/all/
- 中文模型：https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese/index.html
- 中英模型：https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese-English/index.html
- 预训练模型（VITS/MeloTTS）：https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html
- RTF 表：https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/rtf.html
- ZipVoice：https://k2-fsa.github.io/sherpa/onnx/tts/zipvoice.html
- PocketTTS：https://k2-fsa.github.io/sherpa/onnx/tts/pocket.html
- Supertonic：https://k2-fsa.github.io/sherpa/onnx/tts/supertonic.html
- KittenTTS：https://k2-fsa.github.io/sherpa/onnx/tts/kitten.html
- C API 构建：https://k2-fsa.github.io/sherpa/onnx/c-api/index.html
- Windows 预编译包：https://k2-fsa.github.io/sherpa/onnx/install/windows/download-pre-compiled.html
- C++ 示例：https://github.com/k2-fsa/sherpa-onnx/tree/master/cxx-api-examples
- 模型 release：https://github.com/k2-fsa/sherpa-onnx/releases/tag/tts-models
- v2.0.1 release：https://github.com/k2-fsa/sherpa-onnx/releases/tag/v2.0.1

**排行榜**
- Artificial Analysis TTS：https://artificialanalysis.ai/text-to-speech/leaderboard/provider-voice
- TTS Arena V2：https://huggingface.co/spaces/TTS-AGI/TTS-Arena-V2

**模型权重**
- Kokoro 82M：https://huggingface.co/hexgrad/Kokoro-82M
- Kokoro v1.1-zh：https://huggingface.co/hexgrad/Kokoro-82M-v1.1-zh
- MeloTTS：https://huggingface.co/myshell-ai/MeloTTS-Chinese
- Fish Audio Drama 3：https://huggingface.co/fishaudio/drama-3
- Ming-3.0-tts：https://huggingface.co/inclusionAI/Ming-3.0-tts-0.5B
