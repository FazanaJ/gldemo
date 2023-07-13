BUILD_DIR=build
include $(N64_INST)/include/n64.mk

src = $(wildcard src/*.c)
src += $(wildcard assets/*.c)
overlay += $(wildcard src/overlays/*.c)
assets_ttf = $(wildcard assets/fonts/*.ttf)
assets_png = $(wildcard assets/textures/*.png) $(wildcard assets/icons/*.png)
assets_wav = $(wildcard assets/sounds/*.wav)
assets_gltf = $(wildcard assets/models/*.glb)
assets_xm1 = $(wildcard assets/xm/*.xm)

assets_conv = $(addprefix filesystem/,$(notdir $(assets_ttf:%.ttf=%.font64))) \
              $(addprefix filesystem/,$(notdir $(assets_png:%.png=%.sprite))) \
              $(addprefix filesystem/,$(notdir $(assets_wav:%.wav=%.wav64))) \
			  $(addprefix filesystem/,$(notdir $(assets_xm1:%.xm=%.xm64))) \
              $(addprefix filesystem/,$(notdir $(assets_gltf:%.glb=%.model64)))

MKSPRITE_FLAGS ?=
MKFONT_FLAGS ?=
AUDIOCONV_FLAGS ?=

all: gldemo.z64

filesystem/%.font64: assets/fonts/%.ttf
	@mkdir -p $(dir $@)
	@echo "    [FONT] $@"
	@$(N64_MKFONT) $(MKFONT_FLAGS) -o filesystem "$<"

filesystem/%.sprite: assets/textures/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) --compress -o "$(dir $@)" "$<"

filesystem/%.sprite: assets/icons/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) --compress -o "$(dir $@)" "$<"

filesystem/%.wav64: assets/sounds/%.wav
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	@$(N64_AUDIOCONV) -o filesystem $<

filesystem/%.xm64: assets/xm/%.xm
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	@$(N64_AUDIOCONV) $(AUDIOCONV_FLAGS) -o filesystem "$<"

filesystem/%.model64: assets/models/%.glb
	@mkdir -p $(dir $@)
	@echo "    [MODEL] $@"
	@$(N64_MKMODEL) -o filesystem $<

$(BUILD_DIR)/gldemo.dfs: $(assets_conv)
$(BUILD_DIR)/gldemo.elf: $(src:%.c=$(BUILD_DIR)/%.o)


gldemo.z64: N64_ROM_TITLE="Smile Emote"
gldemo.z64: $(BUILD_DIR)/gldemo.dfs

clean:
	rm -rf $(BUILD_DIR) gldemo.z64
	rm -r ./filesystem

clean_src:
	rm -rf $(BUILD_DIR)/src gldemo.z64

-include $(src:%.c=$(BUILD_DIR)/%.d)

.PHONY: all clean
