BUILD_DIR=build
include $(N64_INST)/include/n64.mk

src = $(wildcard src/*.c)
src += $(wildcard assets/*.c)
overlay = $(wildcard src/overlays/*.c)
scene = $(wildcard src/scenes/*.c)
assets_ttf = $(wildcard assets/fonts/*.ttf) $(wildcard assets/archives/*.ttf)
assets_png = $(wildcard assets/textures/character/*.png) $(wildcard assets/textures/environment/*.png) $(wildcard assets/archives/*.png) \
			 $(wildcard assets/textures/hud/*.png) $(wildcard assets/textures/icons/*.png) $(wildcard assets/textures/misc/*.png) \
			 $(wildcard assets/textures/props/*.png) $(wildcard assets/textures/skyboxes/*.png) $(wildcard assets/textures/talksprites/*.png)
assets_wav = $(wildcard assets/sounds/*.wav)
assets_gltf = $(wildcard assets/models/*.glb) $(wildcard assets/archives/*.glb)
assets_xm1 = $(wildcard assets/xm/*.xm)
assets_opus = $(wildcard assets/opus/*.wav)


MAIN_ELF_EXTERNS := $(BUILD_DIR)/gldemo.externs
DSO_MODULES = boot.dso projectile.dso player.dso npc.dso intro.dso testarea.dso testarea2.dso crate.dso barrel.dso testsphere.dso
DSO_LIST = $(addprefix filesystem/, $(DSO_MODULES))

assets_conv = $(addprefix filesystem/,$(notdir $(assets_ttf:%.ttf=%.font64))) \
              $(addprefix filesystem/,$(notdir $(assets_png:%.png=%.sprite))) \
              $(addprefix filesystem/,$(notdir $(assets_wav:%.wav=%.wav64))) \
			  $(addprefix filesystem/,$(notdir $(assets_xm1:%.xm=%.xm64))) \
              $(addprefix filesystem/,$(notdir $(assets_gltf:%.glb=%.model64))) \
              $(addprefix filesystem/,$(notdir $(assets_opus:%.wav=%.wav64))) \
			  $(addprefix filesystem/, $(DSO_MODULES))

MKSPRITE_FLAGS ?=
MKFONT_FLAGS ?=
AUDIOCONV_FLAGS ?= --wav-compress 1
COMPRESS_LEVEL ?= --compress 2

all: gldemo.z64

filesystem/arial.10.font64: MKFONT_FLAGS+=--size 10

filesystem/%.font64: assets/fonts/%.ttf
	@mkdir -p $(dir $@)
	@echo "    [FONT] $@"
	@$(N64_MKFONT) $(MKFONT_FLAGS) $(COMPRESS_LEVEL) -o filesystem "$<"

filesystem/%.font64: assets/archives/%.ttf
	@mkdir -p $(dir $@)
	@echo "    [FONT] $@"
	@$(N64_MKFONT) $(MKFONT_FLAGS) $(COMPRESS_LEVEL) -o filesystem "$<"

filesystem/%.sprite: assets/textures/character/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) $(COMPRESS_LEVEL) -o "$(dir $@)" "$<"

filesystem/%.sprite: assets/textures/environment/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) $(COMPRESS_LEVEL) -o "$(dir $@)" "$<"

filesystem/%.sprite: assets/textures/hud/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) $(COMPRESS_LEVEL) -o "$(dir $@)" "$<"

filesystem/%.sprite: assets/textures/icons/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) $(COMPRESS_LEVEL) -o "$(dir $@)" "$<"

filesystem/%.sprite: assets/textures/misc/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) $(COMPRESS_LEVEL) -o "$(dir $@)" "$<"

filesystem/%.sprite: assets/textures/props/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) $(COMPRESS_LEVEL) -o "$(dir $@)" "$<"

filesystem/%.sprite: assets/textures/skyboxes/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) $(COMPRESS_LEVEL) -o "$(dir $@)" "$<"

filesystem/%.sprite: assets/archives/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) --compress 3 -o "$(dir $@)" "$<"

filesystem/%.wav64: assets/sounds/%.wav
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	@$(N64_AUDIOCONV) --wav-compress 1 -o filesystem $<

filesystem/%.wav64: assets/opus/%.wav
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	@$(N64_AUDIOCONV) --wav-compress 3 --wav-resample 12000 -o filesystem $<

filesystem/%.xm64: assets/xm/%.xm
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	@$(N64_AUDIOCONV) --wav-compress 1 -o filesystem "$<"

filesystem/%.model64: assets/models/%.glb
	@mkdir -p $(dir $@)
	@echo "    [MODEL] $@"
	@$(N64_MKMODEL) $(COMPRESS_LEVEL) -o filesystem $<

filesystem/%.model64: assets/archives/%.glb
	@mkdir -p $(dir $@)
	@echo "    [MODEL] $@"
	@$(N64_MKMODEL) $(COMPRESS_LEVEL) -o filesystem $<

$(BUILD_DIR)/gldemo.dfs: $(assets_conv) $(DSO_LIST)
$(BUILD_DIR)/gldemo.elf: $(src:%.c=$(BUILD_DIR)/%.o) $(MAIN_ELF_EXTERNS)
$(MAIN_ELF_EXTERNS): $(DSO_LIST)

n64brew_SRC = src/overlays/boot.c
filesystem/boot.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/overlays/projectile.c
filesystem/projectile.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/overlays/player.c
filesystem/player.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/overlays/npc.c
filesystem/npc.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/overlays/crate.c
filesystem/crate.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/overlays/barrel.c
filesystem/barrel.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/overlays/testsphere.c
filesystem/testsphere.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)

n64brew_SRC = src/scenes/intro.c
filesystem/intro.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/scenes/testarea.c
filesystem/testarea.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/scenes/testarea2.c
filesystem/testarea2.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)

gldemo.z64: N64_ROM_TITLE="Smile Emote"
gldemo.z64: N64_ROM_SAVETYPE = eeprom16k
gldemo.z64: $(BUILD_DIR)/gldemo.dfs $(BUILD_DIR)/gldemo.msym

$(BUILD_DIR)/gldemo.msym: $(BUILD_DIR)/gldemo.elf

clean:
	rm -rf $(BUILD_DIR) $(DSO_LIST) gldemo.z64
	rm -r ./filesystem

clean_src:
	rm -rf $(BUILD_DIR)/src $(DSO_LIST) gldemo.z64 

-include $(src:%.c=$(BUILD_DIR)/%.d) $(overlay:%.c=$(BUILD_DIR)/%.d)

.PHONY: all clean
