BUILD_DIR=build
include $(N64_INST)/include/n64.mk

src = $(wildcard src/*.c)
src += $(wildcard assets/*.c)
overlay += $(wildcard src/overlays/*.c)
assets_ttf = $(wildcard assets/Fonts/*.ttf)
assets_png = $(wildcard assets/Textures/*.png)
assets_wav = $(wildcard assets/Sounds/*.wav)

assets_conv = $(addprefix filesystem/,$(notdir $(assets_ttf:%.ttf=%.font64))) \
              $(addprefix filesystem/,$(notdir $(assets_png:%.png=%.sprite))) \
              $(addprefix filesystem/,$(notdir $(assets_wav:%.wav=%.wav64)))

MKSPRITE_FLAGS ?=
MKFONT_FLAGS ?=

all: gldemo.z64

filesystem/%.font64: assets/Fonts/%.ttf
	@mkdir -p $(dir $@)
	@echo "    [FONT] $@"
	@$(N64_MKFONT) $(MKFONT_FLAGS) -o filesystem "$<"

filesystem/%.sprite: assets/textures/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) --compress -o "$(dir $@)" "$<"

filesystem/%.wav64: assets/Sounds/%.wav
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	@$(N64_AUDIOCONV) -o filesystem $<

	
filesystem/health.i8.sprite: MKSPRITE_FLAGS=--format I8 --tiles 16,16

$(BUILD_DIR)/gldemo.dfs: $(assets_conv)
$(BUILD_DIR)/gldemo.elf: $(src:%.c=$(BUILD_DIR)/%.o)


gldemo.z64: N64_ROM_TITLE="Smile Emote"
gldemo.z64: $(BUILD_DIR)/gldemo.dfs

clean:
	rm -rf $(BUILD_DIR) gldemo.z64
	rm -r ./filesystem

clean_src:
	rm -rf $(BUILD_DIR)/src gldemo.z64

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean
