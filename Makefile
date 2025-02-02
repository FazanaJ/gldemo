BUILD_DIR=build
T3D_INST ?= ../t3d
include $(N64_INST)/include/n64.mk
include $(T3D_INST)/t3d.mk

FAST_COMPILE = true
ASSET_LEVEL_COMPRESS = 2
ARCHIVE_LEVEL_COMPRESS = 3
SAVETYPE = eeprom16k
ifeq ($(FAST_COMPILE),true)
  N64_ROM_ELFCOMPRESS = 1
  DSO_COMPRESS_LEVEL = 1
else
  N64_ROM_ELFCOMPRESS = 3
  DSO_COMPRESS_LEVEL = 2
endif

src = $(wildcard src/*.c)
src += $(wildcard assets/*.c)
objects = $(wildcard src/objects/*.c)
overlay = $(wildcard src/overlays/*.c)
scene = $(wildcard src/scenes/*.c)
assets_ttf = $(wildcard assets/fonts/*.ttf) $(wildcard assets/archives/*.ttf)
assets_png = $(wildcard assets/textures/character/*.png) $(wildcard assets/textures/environment/*.png) $(wildcard assets/archives/*.png) \
			 $(wildcard assets/textures/hud/*.png) $(wildcard assets/textures/icons/*.png) $(wildcard assets/textures/misc/*.png) \
			 $(wildcard assets/textures/props/*.png) $(wildcard assets/textures/skyboxes/*.png) $(wildcard assets/textures/talksprites/*.png) \
			 $(wildcard assets/textures/maps/*.png)
assets_wav = $(wildcard assets/sounds/lq/*.wav) $(wildcard assets/sounds/mq/*.wav) $(wildcard assets/sounds/hq/*.wav)
assets_gltf = $(wildcard assets/models/*.glb) $(wildcard assets/archives/*.glb)
assets_xm1 = $(wildcard assets/xm/*.xm)
assets_opus = $(wildcard assets/opus/*.wav)

C_DEFINES := $(foreach d,$(DEFINES),-D$(d))
DEF_INC_CFLAGS := $(foreach i,$(INCLUDE_DIRS),-I$(i)) $(C_DEFINES)

N64_CFLAGS += $(DEF_INC_CFLAGS) \
	-Ofast \
	-Werror=double-promotion \
	-mno-check-zero-division \
	-funsafe-math-optimizations \
	-fsingle-precision-constant \
	-falign-functions=32 \
	-fno-merge-constants \
    -fno-strict-aliasing \
	-ffast-math \
    -mips3 \


#$(BUILD_DIR)/src/overlays/boot.o: N64_CFLAGS += -Os
#$(BUILD_DIR)/src/overlays/keyboard.o: N64_CFLAGS += -Os
#$(BUILD_DIR)/src/overlays/options.o: N64_CFLAGS += -Os
#$(BUILD_DIR)/src/overlays/pakmenu.o: N64_CFLAGS += -Os

#$(BUILD_DIR)/src/assets.o: N64_CFLAGS += -Os
#$(BUILD_DIR)/src/screenshot.o: N64_CFLAGS += -Os
#$(BUILD_DIR)/src/save.o: N64_CFLAGS += -Os
#$(BUILD_DIR)/src/menu.o: N64_CFLAGS += -Os
#$(BUILD_DIR)/src/input.o: N64_CFLAGS += -Os
#$(BUILD_DIR)/src/main.o: N64_CFLAGS += -Os
#$(BUILD_DIR)/src/scene.o: N64_CFLAGS += -Os
#$(BUILD_DIR)/src/talk.o: N64_CFLAGS += -Os

MAIN_ELF_EXTERNS := $(BUILD_DIR)/gldemo.externs

DSO_MODULES = boot.dso \
	projectile.dso \
	player.dso \
	npc.dso \
	intro.dso \
	testarea.dso \
	testarea2.dso \
	crate.dso \
	barrel.dso \
	testsphere.dso \
	testarea3.dso \
	healthbar.dso \
	minimap.dso \
	options.dso \
	pakmenu.dso \
	keyboard.dso \
	lightsource.dso \
	testarea4.dso

DSO_LIST = $(addprefix filesystem/, $(DSO_MODULES))

assets_conv = $(addprefix filesystem/,$(notdir $(assets_ttf:%.ttf=%.font64))) \
              $(addprefix filesystem/,$(notdir $(assets_png:%.png=%.sprite))) \
              $(addprefix filesystem/,$(notdir $(assets_wav:%.wav=%.wav64))) \
			  $(addprefix filesystem/,$(notdir $(assets_xm1:%.xm=%.xm64))) \
              $(addprefix filesystem/,$(notdir $(assets_opus:%.wav=%.wav64))) \
			  $(addprefix filesystem/,$(notdir $(assets_gltf:%.glb=%.t3dm))) \
			  $(addprefix filesystem/, $(DSO_MODULES))

MKSPRITE_FLAGS ?=
MKFONT_FLAGS ?=
AUDIOCONV_FLAGS ?= --wav-compress 1
COMPRESS_LEVEL ?= --compress $(ASSET_LEVEL_COMPRESS)

all: gldemo.z64

filesystem/arial.10.font64: MKFONT_FLAGS+=--size 10
filesystem/mvboli.12.font64: MKFONT_FLAGS+=--size 12

filesystem/%.font64: assets/fonts/%.ttf
	@mkdir -p $(dir $@)
	@echo "    [FONT] $@"
	@$(N64_MKFONT) $(MKFONT_FLAGS) $(COMPRESS_LEVEL) -o filesystem "$<"

filesystem/%.font64: assets/archives/%.ttf
	@mkdir -p $(dir $@)
	@echo "    [FONT] $@"
	@$(N64_MKFONT) $(MKFONT_FLAGS) --compress $(ARCHIVE_LEVEL_COMPRESS) -o filesystem "$<"

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

filesystem/%.sprite: assets/textures/maps/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) --compress 3 -o "$(dir $@)" "$<"

filesystem/%.sprite: assets/archives/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) --compress 3 -o "$(dir $@)" "$<"

filesystem/%.wav64: assets/sounds/lq/%.wav
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	@$(N64_AUDIOCONV) --wav-compress 1 --wav-resample 12000 --wav-mono -o filesystem $<

filesystem/%.wav64: assets/sounds/mq/%.wav
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	@$(N64_AUDIOCONV) --wav-compress 1 --wav-resample 16000 --wav-mono -o filesystem $<

filesystem/%.wav64: assets/sounds/hq/%.wav
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	@$(N64_AUDIOCONV) --wav-compress 1 --wav-resample 20000 --wav-mono -o filesystem $<

filesystem/%.wav64: assets/opus/%.wav
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	@$(N64_AUDIOCONV) --wav-compress 3 --wav-resample 12000 --wav-mono -o filesystem $<

filesystem/%.xm64: assets/xm/%.xm
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	@$(N64_AUDIOCONV) --wav-compress 1 --wav-resample 16000 --wav-mono -o filesystem "$<"

filesystem/%.t3dm: assets/models/%.glb
	@mkdir -p $(dir $@)
	@echo "    [MODEL] $@"
	$(T3D_GLTF_TO_3D) "$<" $@ --ignore-materials --base-scale=34
	$(N64_BINDIR)/mkasset $(COMPRESS_LEVEL) -o filesystem $@

filesystem/%.t3dm: assets/archives/%.glb
	@mkdir -p $(dir $@)
	@echo "    [MODEL] $@"
	$(T3D_GLTF_TO_3D) "$<" $@ --ignore-materials --base-scale=34
	$(N64_BINDIR)/mkasset --compress $(ARCHIVE_LEVEL_COMPRESS) -o filesystem $@

$(BUILD_DIR)/gldemo.dfs: $(assets_conv) $(DSO_LIST)
$(BUILD_DIR)/gldemo.elf: $(src:%.c=$(BUILD_DIR)/%.o) $(MAIN_ELF_EXTERNS)
$(MAIN_ELF_EXTERNS): $(DSO_LIST)

n64brew_SRC = src/overlays/boot.c
filesystem/boot.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/overlays/healthbar.c
filesystem/healthbar.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/overlays/minimap.c
filesystem/minimap.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/overlays/options.c
filesystem/options.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/overlays/pakmenu.c
filesystem/pakmenu.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/overlays/keyboard.c
filesystem/keyboard.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)

n64brew_SRC = src/objects/projectile.c
filesystem/projectile.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/objects/player.c
filesystem/player.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/objects/npc.c
filesystem/npc.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/objects/crate.c
filesystem/crate.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/objects/barrel.c
filesystem/barrel.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/objects/testsphere.c
filesystem/testsphere.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/objects/lightsource.c
filesystem/lightsource.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)

n64brew_SRC = src/scenes/intro.c
filesystem/intro.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/scenes/testarea.c
filesystem/testarea.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/scenes/testarea2.c
filesystem/testarea2.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/scenes/testarea3.c
filesystem/testarea3.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)
n64brew_SRC = src/scenes/testarea4.c
filesystem/testarea4.dso: $(n64brew_SRC:%.c=$(BUILD_DIR)/%.o)

gldemo.z64: N64_ROM_TITLE="Smile Emote"
gldemo.z64: N64_ROM_SAVETYPE = $(SAVETYPE)
gldemo.z64: $(BUILD_DIR)/gldemo.dfs $(BUILD_DIR)/gldemo.msym

$(BUILD_DIR)/gldemo.msym: $(BUILD_DIR)/gldemo.elf

clean:
	rm -rf $(BUILD_DIR) $(DSO_LIST) gldemo.z64
	rm -r ./filesystem

clean_src:
	rm -rf $(BUILD_DIR)/src $(DSO_LIST) gldemo.z64 

-include $(src:%.c=$(BUILD_DIR)/%.d) $(overlay:%.c=$(BUILD_DIR)/%.d)

.PHONY: all clean
