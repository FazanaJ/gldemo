from PyQt5 import QtWidgets, QtCore
from PyQt5.QtWidgets import QApplication, QWidget, QMainWindow, QListWidget, QListWidgetItem, QLabel, QFileDialog
from PyQt5.QtGui import QPixmap
import configparser
import sys
import re
import os

def check_valid_directory():
    if (os.path.exists(app.rootDir + "/assets/textures")):
        print("Texture directory found.")
        return True
    else:
        print("Texture directory missing.")
        return False
    
def write_config():
    with open(app.configDir, "w") as configfile:
        app.config.write(configfile)

def boot_config():
    if not os.path.exists(app.configDir):
        print("No config found, generating new one.")
        app.config.add_section("Core")
        app.config["Core"]["version"] = "1.0"
        app.config["Core"]["last_directory"] = app.rootDir
        write_config()
    else:
        app.config.read("toolconfig.ini")
        app.rootDir = app.config["Core"]["last_directory"]
    if not (os.path.exists(app.rootDir + "/assets/textures")):
        app.rootDir = QFileDialog.getExistingDirectory(window, "Open Repo Folder", app.rootDir, QtWidgets.QFileDialog.ShowDirsOnly)
        window.setWindowTitle("Texedit | " + app.rootDir)
        app.config["Core"]["last_directory"] = app.rootDir
        write_config()
    
class Ui_MainWindow(object):
    def setupUi(self, MainWindow):
        MainWindow.setObjectName("MainWindow")
        MainWindow.setWindowTitle("MainWindow")
        MainWindow.resize(640, 480)
        self.centralwidget = QtWidgets.QWidget(MainWindow)
        self.centralwidget.setObjectName("centralwidget")
        MainWindow.setCentralWidget(self.centralwidget)
        QtCore.QMetaObject.connectSlotsByName(MainWindow)


def window_resize():
    window.texList.resize(140, window.frameGeometry().height() - 22 - 112)
    window.saveTexButton.move(2, window.frameGeometry().height() - 20 - 80)
    window.newTexButton.move(2, window.frameGeometry().height() - 20 - 56)
    window.deleteTexButton.move(2, window.frameGeometry().height() - 20 - 32)
    window.matList.resize(140, window.frameGeometry().height() - 22 - 112)
    window.saveMatButton.move(2, window.frameGeometry().height() - 20 - 80)
    window.newMatButton.move(2, window.frameGeometry().height() - 20 - 56)
    window.deleteMatButton.move(2, window.frameGeometry().height() - 20 - 32)
    
class Window(QtWidgets.QMainWindow):
    resized = QtCore.pyqtSignal()
    def  __init__(self, parent=None):
        super(Window, self).__init__(parent=parent)
        ui = Ui_MainWindow()
        ui.setupUi(self)

    def resizeEvent(self, event):
        window_resize()
        self.resized.emit()

        return super(Window, self).resizeEvent(event)

app = QApplication(sys.argv)
app.page = 0
app.elementY = 0
app.textureNames = []
app.textureEnums = []
app.textureClampH = []
app.textureClampV = []
app.textureMirrorH = []
app.textureMirrorV = []
app.textureFlipbookFrames = []
app.textureFlipbookSpeed = []
app.textureCategory = []
app.textureCount = 0

app.materialSoundStrings = ["None", "Dirt", "Grass", "Stone", "Gravel", "Tile", "Wood", "Glass", "Water", "Mesh", "Sand", "Snow", "Metal", "Carpet"]
app.materialSoundEnums = ["None", "COLFLAG_SOUND_DIRT", "COLFLAG_SOUND_GRASS", "COLFLAG_SOUND_STONE", "COLFLAG_SOUND_GRAVEL", "COLFLAG_SOUND_TILE", "COLFLAG_SOUND_WOOD", "COLFLAG_SOUND_GLASS", "COLFLAG_SOUND_WATER", "COLFLAG_SOUND_MESH", "COLFLAG_SOUND_SAND", "COLFLAG_SOUND_SNOW", "COLFLAG_SOUND_METAL", "COLFLAG_SOUND_CARPET"]

app.materialNames = []
app.materialEnums = []
app.materialTex0 = []
app.materialTex1 = []
app.materialCombiner = []
app.materialShiftS0 = []
app.materialShiftT0 = []
app.materialShiftS1 = []
app.materialShiftT1 = []
app.materialMoveS0 = []
app.materialMoveT0 = []
app.materialMoveS1 = []
app.materialMoveT1 = []

app.renderCutout = []
app.renderXlu = []
app.renderLighting = []
app.renderFog = []
app.renderEnvmap = []
app.renderDepth = []
app.renderVtxcol = []
app.renderDecal = []
app.renderInter = []
app.renderBackface = []
app.renderInvis = []
app.renderFrontface = []
app.renderCI = []

app.colSound = []
app.colGrip = []
app.colIntangible = []
app.colNocam = []
app.colCamonly = []
app.colShadow = []

app.materialCount = 0

app.combinerNames = []
app.texSelection = 0
app.matSelection = 0
window = Window()
app.config = configparser.ConfigParser()
app.rootDir = ""
app.configDir = "./toolconfig.ini"

def create_button(parent, x, y, w, h, text, hidden):
    button = QtWidgets.QPushButton(parent)
    button.setText(text)
    button.move(x, y)
    button.resize(w, h)
    button.setVisible(hidden)
    app.elementY += h + 2
    return button

def create_label(parent, x, y, w, h, text, hidden):
    button = QtWidgets.QLabel(parent)
    button.setText(text)
    button.move(x, y)
    button.resize(w, h)
    button.setVisible(hidden)
    #button.setAlignment(Qt.AlignCenter)
    app.elementY += h + 2
    return button

def create_tickbox(parent, x, y, w, h, text, hidden):
    button = QtWidgets.QCheckBox(parent)
    button.setText(text)
    button.move(x, y)
    button.resize(w, h)
    button.setVisible(hidden)
    app.elementY += h + 2
    return button

def create_input(parent, x, y, w, h, hidden):
    button = QtWidgets.QLineEdit(parent)
    button.move(x, y)
    button.resize(w, h)
    #button.setAlignment(Qt.AlignCenter)
    button.setVisible(hidden)
    app.elementY += h + 2
    return button

def create_combobox(parent, x, y, w, h, hidden):
    button = QtWidgets.QComboBox(parent)
    button.move(x, y)
    button.resize(w, h)
    button.setVisible(hidden)
    app.elementY += h + 4
    return button

def create_slider(parent, x, y, w, h, max, align, hidden):
    button = QtWidgets.QSlider(align, parent)
    button.move(x, y)
    button.resize(w, h)
    button.setMaximum(max)
    button.setVisible(hidden)
    app.elementY += h + 2
    return button

def switch_window_tex():
    window.saveTexButton.setVisible(True)
    window.newTexButton.setVisible(True)
    window.deleteTexButton.setVisible(True)
    window.texImage.setVisible(True)
    window.texNameInput.setVisible(True)
    window.texName.setVisible(True)
    window.texList.setVisible(True)
    window.clampH.setVisible(True)
    window.clampV.setVisible(True)
    window.mirrorH.setVisible(True)
    window.mirrorV.setVisible(True)
    window.texCategory.setVisible(True)
    window.texFlipbookInput.setVisible(True)
    window.texFlipbookName.setVisible(True)
    window.texFlipSpeedInput.setVisible(True)
    window.texFlipSpeedName.setVisible(True)
    
    window.matList.setVisible(False)
    window.saveMatButton.setVisible(False)
    window.newMatButton.setVisible(False)
    window.deleteMatButton.setVisible(False)
    window.matNameLabel.setVisible(False)
    window.matNameInput.setVisible(False)
    window.matTex0Label.setVisible(False)
    window.matTex0.setVisible(False)
    window.matTex1Label.setVisible(False)
    window.matTex1.setVisible(False)
    window.matCombinerLabel.setVisible(False)
    window.matCombiner.setVisible(False)
    window.matShiftS0Label.setVisible(False)
    window.matShiftS0.setVisible(False)
    window.matShiftS1Label.setVisible(False)
    window.matShiftS1.setVisible(False)
    window.matShiftT0Label.setVisible(False)
    window.matShiftT0.setVisible(False)
    window.matShiftT1Label.setVisible(False)
    window.matShiftT1.setVisible(False)
    window.matScrollS0Label.setVisible(False)
    window.matScrollS0.setVisible(False)
    window.matScrollS1Label.setVisible(False)
    window.matScrollS1.setVisible(False)
    window.matScrollT0Label.setVisible(False)
    window.matScrollT0.setVisible(False)
    window.matScrollT1Label.setVisible(False)
    window.matScrollT1.setVisible(False)
    window.matFlagsLabels.setVisible(False)
    window.matFlagCutout.setVisible(False)
    window.matFlagXlu.setVisible(False)
    window.matFlagLighting.setVisible(False)
    window.matFlagFog.setVisible(False)
    window.matFlagEnvmap.setVisible(False)
    window.matFlagZBuffer.setVisible(False)
    window.matFlagVtxCol.setVisible(False)
    window.matFlagDecal.setVisible(False)
    window.matFlagInter.setVisible(False)
    window.matFlagBackface.setVisible(False)
    window.matFlagInvis.setVisible(False)
    window.matFlagFrontface.setVisible(False)
    window.matFlagCI.setVisible(False)
    window.matColLabel.setVisible(False)
    window.matColSoundLabel.setVisible(False)
    window.matColSoundInput.setVisible(False)
    window.matColGripLabel.setVisible(False)
    window.matColGripInput.setVisible(False)
    window.matColIntangible.setVisible(False)
    window.matColNocam.setVisible(False)
    window.matColCamonly.setVisible(False)
    window.matColShadow.setVisible(False)
        
def switch_window_mat():
    window.saveTexButton.setVisible(False)
    window.newTexButton.setVisible(False)
    window.deleteTexButton.setVisible(False)
    window.texImage.setVisible(False)
    window.texNameInput.setVisible(False)
    window.texName.setVisible(False)
    window.texList.setVisible(False)
    window.clampH.setVisible(False)
    window.clampV.setVisible(False)
    window.mirrorH.setVisible(False)
    window.mirrorV.setVisible(False)
    window.texCategory.setVisible(False)
    window.texFlipbookInput.setVisible(False)
    window.texFlipbookName.setVisible(False)
    window.texFlipSpeedInput.setVisible(False)
    window.texFlipSpeedName.setVisible(False)

    window.matList.setVisible(True)
    window.saveMatButton.setVisible(True)
    window.newMatButton.setVisible(True)
    window.deleteMatButton.setVisible(True)
    window.matNameLabel.setVisible(True)
    window.matNameInput.setVisible(True)
    window.matTex0Label.setVisible(True)
    window.matTex0.setVisible(True)
    window.matTex1Label.setVisible(True)
    window.matTex1.setVisible(True)
    window.matCombinerLabel.setVisible(True)
    window.matCombiner.setVisible(True)
    window.matShiftS0Label.setVisible(True)
    window.matShiftS0.setVisible(True)
    window.matShiftS1Label.setVisible(True)
    window.matShiftS1.setVisible(True)
    window.matShiftT0Label.setVisible(True)
    window.matShiftT0.setVisible(True)
    window.matShiftT1Label.setVisible(True)
    window.matShiftT1.setVisible(True)
    window.matScrollS0Label.setVisible(True)
    window.matScrollS0.setVisible(True)
    window.matScrollS1Label.setVisible(True)
    window.matScrollS1.setVisible(True)
    window.matScrollT0Label.setVisible(True)
    window.matScrollT0.setVisible(True)
    window.matScrollT1Label.setVisible(True)
    window.matScrollT1.setVisible(True)
    window.matFlagsLabels.setVisible(True)
    window.matFlagCutout.setVisible(True)
    window.matFlagXlu.setVisible(True)
    window.matFlagLighting.setVisible(True)
    window.matFlagFog.setVisible(True)
    window.matFlagEnvmap.setVisible(True)
    window.matFlagZBuffer.setVisible(True)
    window.matFlagVtxCol.setVisible(True)
    window.matFlagDecal.setVisible(True)
    window.matFlagInter.setVisible(True)
    window.matFlagBackface.setVisible(True)
    window.matFlagInvis.setVisible(True)
    window.matFlagFrontface.setVisible(True)
    window.matFlagCI.setVisible(True)
    window.matColLabel.setVisible(True)
    window.matColSoundLabel.setVisible(True)
    window.matColSoundInput.setVisible(True)
    window.matColGripLabel.setVisible(True)
    window.matColGripInput.setVisible(True)
    window.matColIntangible.setVisible(True)
    window.matColNocam.setVisible(True)
    window.matColCamonly.setVisible(True)
    window.matColShadow.setVisible(True)

def make_window():
    window.setWindowTitle("Texture Manager")
    window.texManagerButton = create_button(window, 2, 4, 140, 24, "Textures", False)
    window.texManagerButton.setVisible(True)
    window.matManagerButton = create_button(window, 144, 4, 140, 24, "Materials", False)
    window.matManagerButton.setVisible(True)

    window.texList = QListWidget(window)
    window.texList.move(0, 32)
    window.saveTexButton = create_button(window, 2, 32, 140, 24, "Save Texture", False)
    window.newTexButton = create_button(window, 2, 32, 140, 24, "Add Texture", False)
    window.deleteTexButton = create_button(window, 2, 32, 140, 24, "Delete Texture", False)
    window.saveTexButton.setVisible(True)
    window.newTexButton.setVisible(True)
    window.deleteTexButton.setVisible(True)
    app.elementY = 32
    window.texNameInput = create_label(window, 160, app.elementY, 160, 16, "Texture Name", True)
    window.texName = create_input(window, 160, app.elementY, 160, 28, True)

    window.clampH = create_tickbox(window, 160, app.elementY, 160, 16, "Clamp H", True)
    window.clampV = create_tickbox(window, 160, app.elementY, 160, 16, "Clamp V", True)
    app.elementY = 80
    window.mirrorH = create_tickbox(window, 240, app.elementY, 160, 16, "Mirror H", True)
    window.mirrorV = create_tickbox(window, 240, app.elementY, 160, 16, "Mirror V", True)
    window.texCategory = create_label(window, 160, app.elementY, 160, 16, "Folder:", True)

    window.texFlipbookInput = create_label(window, 160, app.elementY, 160, 16, "Flipbook Frame Count", True)
    window.texFlipbookName = create_input(window, 160, app.elementY, 160, 28, True)
    window.texFlipSpeedInput = create_label(window, 160, app.elementY, 160, 16, "Flipbook Speed", True)
    window.texFlipSpeedName = create_input(window, 160, app.elementY, 160, 28, True)

    window.texImage = QtWidgets.QLabel(window)
    window.texImage.move(160, app.elementY)
    window.texImage.setVisible(True)

    window.matList = QListWidget(window)
    window.matList.move(0, 32)
    window.matList.setVisible(False)
    window.saveMatButton = create_button(window, 2, 32, 140, 24, "Save Material", False)
    window.newMatButton = create_button(window, 2, 32, 140, 24, "Add Material", False)
    window.deleteMatButton = create_button(window, 2, 32, 140, 24, "Delete Material", False)
    app.elementY = 32
    window.matNameLabel = create_label(window, 160, app.elementY, 160, 16, "Material Name", False)
    window.matNameInput = create_input(window, 160, app.elementY, 160, 28, False)
    window.matTex0Label = create_label(window, 160, app.elementY, 160, 16, "Texture 0", False)
    window.matTex0 = create_combobox(window, 160, app.elementY, 160, 28, False)
    window.matTex1Label = create_label(window, 160, app.elementY, 160, 16, "Texture 1", False)
    window.matTex1 = create_combobox(window, 160, app.elementY, 160, 28, False)
    window.matCombinerLabel = create_label(window, 160, app.elementY, 160, 16, "Combiner", False)
    window.matCombiner = create_combobox(window, 160, app.elementY, 160, 28, False)
    window.matShiftS0Label = create_label(window, 160, app.elementY, 72, 16, "Tex0 Shift S", False)
    window.matShiftS0 = create_input(window, 160, app.elementY, 72, 28, False)
    window.matShiftT0Label = create_label(window, 160, app.elementY, 72, 16, "Tex0 Shift T", False)
    window.matShiftT0 = create_input(window, 160, app.elementY, 72, 28, False)
    window.matScrollS0Label = create_label(window, 160, app.elementY, 72, 16, "Tex0 Scroll S", False)
    window.matScrollS0 = create_input(window, 160, app.elementY, 72, 28, False)
    window.matScrollT0Label = create_label(window, 160, app.elementY, 72, 16, "Tex0 Scroll T", False)
    window.matScrollT0 = create_input(window, 160, app.elementY, 72, 28, False)
    app.elementY = 230
    window.matShiftS1Label = create_label(window, 240, app.elementY, 72, 16, "Tex1 Shift S", False)
    window.matShiftS1 = create_input(window, 240, app.elementY, 72, 28, False)
    window.matShiftT1Label = create_label(window, 240, app.elementY, 72, 16, "Tex1 Shift T", False)
    window.matShiftT1 = create_input(window, 240, app.elementY, 72, 28, False)
    window.matScrollS1Label = create_label(window, 240, app.elementY, 72, 16, "Tex1 Scroll S", False)
    window.matScrollS1 = create_input(window, 240, app.elementY, 72, 28, False)
    window.matScrollT1Label = create_label(window, 240, app.elementY, 72, 16, "Tex1 Scroll T", False)
    window.matScrollT1 = create_input(window, 240, app.elementY, 72, 28, False)
    app.elementY = 32
    window.matFlagsLabels = create_label(window, 336, app.elementY, 160, 16, "Render Flags", False)
    window.matFlagCutout = create_tickbox(window, 336, app.elementY, 160, 16, "Cutout", False)
    window.matFlagXlu = create_tickbox(window, 336, app.elementY, 160, 16, "Semitransparent", False)
    window.matFlagLighting = create_tickbox(window, 336, app.elementY, 160, 16, "Lighting", False)
    window.matFlagFog = create_tickbox(window, 336, app.elementY, 160, 16, "Fog", False)
    window.matFlagEnvmap = create_tickbox(window, 336, app.elementY, 160, 16, "Env Mapping", False)
    window.matFlagZBuffer = create_tickbox(window, 336, app.elementY, 160, 16, "ZBuffer", False)
    window.matFlagVtxCol = create_tickbox(window, 336, app.elementY, 160, 16, "Vertex Colours", False)
    window.matFlagDecal = create_tickbox(window, 336, app.elementY, 160, 16, "Decal", False)
    window.matFlagInter = create_tickbox(window, 336, app.elementY, 160, 16, "Interpenetrating", False)
    window.matFlagBackface = create_tickbox(window, 336, app.elementY, 160, 16, "Back Faces", False)
    window.matFlagInvis = create_tickbox(window, 336, app.elementY, 160, 16, "Invisible", False)
    window.matFlagFrontface = create_tickbox(window, 336, app.elementY, 160, 16, "Front Faces", False)
    window.matFlagCI = create_tickbox(window, 336, app.elementY, 160, 16, "Colour Index", False)
    app.elementY = 32
    window.matColLabel = create_label(window, 460, app.elementY, 160, 16, "Collision", False)
    window.matColSoundLabel = create_label(window, 460, app.elementY, 160, 16, "Sound", False)
    window.matColSoundInput = create_combobox(window, 460, app.elementY, 160, 28, False)
    window.matColSoundInput.addItems(app.materialSoundStrings);
    window.matColGripLabel = create_label(window, 460, app.elementY, 160, 16, "Grip: 0", False)
    window.matColGripInput = create_slider(window, 460, app.elementY, 160, 28, 15, QtCore.Qt.Horizontal, False)
    window.matColIntangible = create_tickbox(window, 460, app.elementY, 160, 16, "Intangible", False)
    window.matColNocam = create_tickbox(window, 460, app.elementY, 160, 16, "Block Camera", False)
    window.matColCamonly = create_tickbox(window, 460, app.elementY, 160, 16, "Camera Only", False)
    window.matColShadow = create_tickbox(window, 460, app.elementY, 160, 16, "Allow Shadow", False)
    window.show()

def init_mat_list():
    file = open(app.rootDir + "/include/material_Table.h")
    levelString = file.readlines()
    file.close()
    enumFound = False
    for lineS in levelString:
        line = lineS.strip()
        if enumFound == False:
            ln2 = line.find("gMaterialIDs")
            if not ln2 == -1:
                enumFound = True
        else:
            ln = line.find("};")
            if ln == -1:
                tex0 = line.partition(",")[0]
                tex0 = tex0[1:]
                flagStr = line.partition(",")[2]
                matches = re.findall(r'[^\s,]+(?:\s\|\s[^\s,]+)*', flagStr[:-2])
                nameStart = line.find("//")
                name = line[nameStart + 3:]
                app.materialNames.insert(app.materialCount, str(name))
                app.materialTex0.insert(app.materialCount, str(tex0))
                app.materialTex1.insert(app.materialCount, str(matches[0]))
                flags = matches[1]
                if (not flags.find("MAT_CUTOUT") == -1):
                    app.renderCutout.insert(app.materialCount, True)
                else:
                    app.renderCutout.insert(app.materialCount, False)
                if (not flags.find("MAT_XLU") == -1):
                    app.renderXlu.insert(app.materialCount, True)
                else:
                    app.renderXlu.insert(app.materialCount, False)
                if (not flags.find("MAT_LIGHTING") == -1):
                    app.renderLighting.insert(app.materialCount, True)
                else:
                    app.renderLighting.insert(app.materialCount, False)
                if (not flags.find("MAT_FOG") == -1):
                    app.renderFog.insert(app.materialCount, True)
                else:
                    app.renderFog.insert(app.materialCount, False)
                if (not flags.find("MAT_ENVMAP") == -1):
                    app.renderEnvmap.insert(app.materialCount, True)
                else:
                    app.renderEnvmap.insert(app.materialCount, False)
                if (not flags.find("MAT_DEPTH_READ") == -1):
                    app.renderDepth.insert(app.materialCount, True)
                else:
                    app.renderDepth.insert(app.materialCount, False)
                if (not flags.find("MAT_VTXCOL") == -1):
                    app.renderVtxcol.insert(app.materialCount, True)
                else:
                    app.renderVtxcol.insert(app.materialCount, False)
                if (not flags.find("MAT_DECAL") == -1):
                    app.renderDecal.insert(app.materialCount, True)
                else:
                    app.renderDecal.insert(app.materialCount, False)
                if (not flags.find("MAT_INTER") == -1):
                    app.renderInter.insert(app.materialCount, True)
                else:
                    app.renderInter.insert(app.materialCount, False)
                if (not flags.find("MAT_BACKFACE") == -1):
                    app.renderBackface.insert(app.materialCount, True)
                else:
                    app.renderBackface.insert(app.materialCount, False)
                if (not flags.find("MAT_INVISIBLE") == -1):
                    app.renderInvis.insert(app.materialCount, True)
                else:
                    app.renderInvis.insert(app.materialCount, False)
                if (not flags.find("MAT_FRONTFACE") == -1):
                    app.renderFrontface.insert(app.materialCount, True)
                else:
                    app.renderFrontface.insert(app.materialCount, False)
                if (not flags.find("MAT_CI") == -1):
                    app.renderCI.insert(app.materialCount, True)
                else:
                    app.renderCI.insert(app.materialCount, False)
                app.materialCombiner.insert(app.materialCount, str(matches[2]))
                app.materialShiftS0.insert(app.materialCount, str(matches[4]))
                app.materialShiftT0.insert(app.materialCount, str(matches[5]))
                app.materialShiftS1.insert(app.materialCount, str(matches[6]))
                app.materialShiftT1.insert(app.materialCount, str(matches[7]))
                app.materialMoveS0.insert(app.materialCount, str(matches[8]))
                app.materialMoveT0.insert(app.materialCount, str(matches[9]))
                app.materialMoveS1.insert(app.materialCount, str(matches[10]))
                app.materialMoveT1.insert(app.materialCount, str(matches[11][:-1]))
                flags = matches[3]
                if (not flags.find("COLFLAG_INTANGIBLE") == -1):
                    app.colIntangible.insert(app.materialCount, True)
                else:
                    app.colIntangible.insert(app.materialCount, False)
                if (not flags.find("COLFLAG_CAM_ONLY") == -1):
                    app.colCamonly.insert(app.materialCount, True)
                else:
                    app.colCamonly.insert(app.materialCount, False)
                if (not flags.find("COLFLAG_NO_CAM") == -1):
                    app.colNocam.insert(app.materialCount, True)
                else:
                    app.colNocam.insert(app.materialCount, False)
                if (not flags.find("COLFLAG_SHADOW") == -1):
                    app.colShadow.insert(app.materialCount, True)
                else:
                    app.colShadow.insert(app.materialCount, False)
                gripStr = flags.find("COLFLAG_GRIP")
                if (not gripStr == -1):
                    app.colGrip.insert(app.materialCount, flags[gripStr + 13])
                else:
                    app.colGrip.insert(app.materialCount, 8)
                soundFlag = flags.find("COLFLAG_SOUND_")
                if (not soundFlag == -1):
                    soundString = flags[soundFlag:]
                    terminator = soundString.find(" ")
                    if (terminator != -1):
                        soundString = soundString[:terminator]
                    i = 0
                    while (i < len(app.materialSoundEnums)):
                        if (str(app.materialSoundEnums[i]) == str(soundString)):
                            app.colSound.insert(app.materialCount, app.materialSoundStrings[i])
                            break
                        i+=1
                else:
                    app.colSound.insert(app.materialCount, "None")
                app.materialCount += 1
    window.matList.addItems(app.materialNames)

def init_tex_list():
    file = open(app.rootDir + "/include/texture_Table.h")
    levelString = file.readlines()
    file.close()
    enumFound = False
    for lineS in levelString:
        line = lineS.strip()
        if enumFound == False:
            ln2 = line.find("gTextureIDs")
            if not ln2 == -1:
                enumFound = True
        else:
            ln = line.find("};")
            ln3 = line.rfind('"')
            if ln == -1:
                name = line[ln + 3:ln3]
                flagStr = line.partition(",")[2]
                matches = re.findall(r'[^\s,]+(?:\s\|\s[^\s,]+)*', flagStr[:-2])
                matchPos = 1
                flags = matches[0]
                category = ""
                addClampH = False
                addClampV = False
                addMirrorH = False
                addMirrorV = False
                useNum = True
                if (not flags.find("TEX_MIRROR_H") == -1):
                    addMirrorH = True
                    useNum = False
                if (not flags.find("TEX_MIRROR_V") == -1):
                    addMirrorV = True
                    useNum = False
                if (not flags.find("TEX_CLAMP_H") == -1):
                    addClampH = True
                    useNum = False
                if (not flags.find("TEX_CLAMP_V") == -1):
                    addClampV = True
                    useNum = False
                if (not flags.find("TEX_CLAMP_V") == -1):
                    addClampV = True
                    useNum = False
                if (not flags.find("TEX_NULL") == -1):
                    useNum = False
                if useNum == True:
                    num = int(flags)
                    if (num >= 8):
                        num -= 8
                        addMirrorV = True
                    if (num >= 4):
                        num -= 4
                        addMirrorH = True
                    if (num >= 2):
                        num -= 2
                        addClampV = True
                    if (num >= 1):
                        num -= 1
                        addClampH = True
                index = len(app.textureNames)
                for dirpath, dirnames, filenames in os.walk(app.rootDir + "/assets/textures"):
                    testName = name + ".png"
                    if testName in filenames:
                        folder_name = os.path.basename(dirpath)
                        app.textureCategory.insert(index, folder_name)
                app.textureClampH.insert(app.textureCount, addClampH)
                app.textureClampV.insert(app.textureCount, addClampV)
                app.textureMirrorH.insert(app.textureCount, addMirrorH)
                app.textureMirrorV.insert(app.textureCount, addMirrorV)
                app.textureFlipbookFrames.insert(app.textureCount, matches[1])
                app.textureFlipbookSpeed.insert(app.textureCount, matches[2])
                app.textureNames.insert(index, name)
                app.textureCount += 1
    enumFound = False
    file = open(app.rootDir + "/include/enums.h")
    levelString = file.readlines()
    file.close()
    for lineS in levelString:
        line = lineS.strip()
        if enumFound == False:
            ln2 = line.find("TextureNames")
            if not ln2 == -1:
                enumFound = True
        else:
            ln = line.find("};")
            ln3 = line.rfind(',')
            if ln == -1:
                name = line[ln + 1:ln3]
                index = len(app.textureEnums)
                app.textureEnums.insert(index, name)
            else:
                break
    enumFound = False
    for lineS in levelString:
        line = lineS.strip()
        if enumFound == False:
            ln2 = line.find("MaterialNames")
            if not ln2 == -1:
                enumFound = True
        else:
            ln = line.find("};")
            ln3 = line.rfind(',')
            if ln == -1:
                name = line[ln + 1:ln3]
                index = len(app.materialEnums)
                app.materialEnums.insert(index, name)
    
    window.texList.addItems(app.textureNames)
    app.materialEnums.pop(len(app.materialEnums) - 1)

    enumFound = False
    file = open(app.rootDir + "/src/assets.h")
    levelString = file.readlines()
    file.close()
    for lineS in levelString:
        line = lineS.strip()
        if enumFound == False:
            ln2 = line.find("CombinerNames")
            if not ln2 == -1:
                enumFound = True
        else:
            ln = line.find("};")
            ln3 = line.rfind(',')
            if ln == -1:
                name = line[ln + 1:ln3]
                index = len(app.combinerNames)
                app.combinerNames.insert(index, name)
            else:
                break
    app.combinerNames.pop(len(app.combinerNames) - 1)
    app.combinerNames.pop(len(app.combinerNames) - 1)
    window.matCombiner.addItems(app.combinerNames)

def write_enums():
    with open(app.rootDir + "/include/enums.h", 'r+') as fp:
    # read an store all lines into list
        lines = fp.readlines()
        # move file pointer to the beginning of a file
        fp.seek(0)
        # truncate the file
        fp.truncate(0)

        fp.write("#pragma once\n\n")
        fp.write("enum TextureNames {\n")
        i = 0
        while (i < len(app.textureEnums)):
            fp.write("    " + str(app.textureEnums[i]) + ",\n")
            i += 1
        fp.write("};\n\n")
        fp.write("enum MaterialNames {\n")
        i = 0
        while (i < len(app.materialEnums)):
            fp.write("    " + str(app.materialEnums[i]) + ",\n")
            i += 1
        fp.write("};\n\n")
        fp.close()

def write_textures():
    with open(app.rootDir + "/include/texture_Table.h", 'r+') as fp:
    # read an store all lines into list
        lines = fp.readlines()
        # move file pointer to the beginning of a file
        fp.seek(0)
        # truncate the file
        fp.truncate()
        firstLine = ""
        # start writing lines
        # iterate line and line number
        for number, line in enumerate(lines):
            # delete line number 5 and 8
            # note: list index start from 0
            if (not line.find("gTextureIDs") == -1):
                firstLine = line
            if (line.find("},") == -1 and line.find("};") == -1):
                fp.write(line)
            
        numLines = 0
        for i in app.textureNames:
            totalFlags = 0
            writeString = ""
            firstTime = False
            if (app.textureClampH[numLines] == True):
                writeString = "TEX_CLAMP_H"
                firstTime = True
                totalFlags += 1
            if (app.textureClampV[numLines] == True):
                if (firstTime == False):
                    writeString = "TEX_CLAMP_V"
                else:
                    writeString += " | TEX_CLAMP_V"
                firstTime = True
                totalFlags += 2
            if (app.textureMirrorH[numLines] == True):
                if (firstTime == False):
                    writeString = "TEX_MIRROR_H"
                else:
                    writeString += " | TEX_MIRROR_H"
                firstTime = True
                totalFlags += 4
            if (app.textureMirrorV[numLines] == True):
                if (firstTime == False):
                    writeString = "TEX_MIRROR_V"
                else:
                    writeString += " | TEX_MIRROR_V"
                firstTime = True
                totalFlags += 8
            flipFrames = int(app.textureFlipbookFrames[numLines])
            if (flipFrames < 2):
                flipFrames = 0
            if (flipFrames > 31):
                flipFrames = 31
            flipSpeed = int(app.textureFlipbookSpeed[numLines])
            if flipFrames == 0:
                flipSpeed = 0
            else:
                if (flipSpeed < 0):
                    flipSpeed = 0
                if (flipSpeed > 7):
                    flipSpeed = 7
            if (totalFlags == 0):
                writeString = "TEX_NULL"
            name = '    {"' + app.textureNames[numLines] + '", ' + str(writeString) + ', ' + str(flipFrames) + ', ' + str(flipSpeed) + '},\n'
            lines.insert(6, name)
            new = "".join(name)
            fp.write(new)
            numLines += 1
        fp.write("};")
        write_enums()
        print("Saved textures!")

def write_materials():
    with open(app.rootDir + "/include/material_table.h", 'r+') as fp:
    # read an store all lines into list
        lines = fp.readlines()
        # move file pointer to the beginning of a file
        fp.seek(0)
        # truncate the file
        fp.truncate()
        firstLine = ""
        # start writing lines
        # iterate line and line number
        for number, line in enumerate(lines):
            # delete line number 5 and 8
            # note: list index start from 0
            if (not line.find("gMaterialIDs") == -1):
                firstLine = line
            if (line.find("},") == -1 and line.find("};") == -1):
                fp.write(line)
            
        numLines = 0
        for i in app.materialNames:
            name = '    {'
            name += str(app.materialTex0[numLines]) + ", "
            name += str(app.materialTex1[numLines]) + ", "
            matFlags = ""
            firstEnum = False
            if (app.renderCutout[numLines] == True):
                firstEnum = True
                matFlags += "MAT_CUTOUT"
            if (app.renderXlu[numLines] == True):
                if (firstEnum == False):
                    firstEnum = True
                else:
                    matFlags += " | "
                matFlags += "MAT_XLU"
            if (app.renderLighting[numLines] == True):
                if (firstEnum == False):
                    firstEnum = True
                else:
                    matFlags += " | "
                matFlags += "MAT_LIGHTING"
            if (app.renderFog[numLines] == True):
                if (firstEnum == False):
                    firstEnum = True
                else:
                    matFlags += " | "
                matFlags += "MAT_FOG"
            if (app.renderEnvmap[numLines] == True):
                if (firstEnum == False):
                    firstEnum = True
                else:
                    matFlags += " | "
                matFlags += "MAT_ENVMAP"
            if (app.renderDepth[numLines] == True):
                if (firstEnum == False):
                    firstEnum = True
                else:
                    matFlags += " | "
                matFlags += "MAT_DEPTH_READ"
            if (app.renderVtxcol[numLines] == True):
                if (firstEnum == False):
                    firstEnum = True
                else:
                    matFlags += " | "
                matFlags += "MAT_VTXCOL"
            if (app.renderDecal[numLines] == True):
                if (firstEnum == False):
                    firstEnum = True
                else:
                    matFlags += " | "
                matFlags += "MAT_DECAL"
            if (app.renderInter[numLines] == True):
                if (firstEnum == False):
                    firstEnum = True
                else:
                    matFlags += " | "
                matFlags += "MAT_INTER"
            if (app.renderBackface[numLines] == True):
                if (firstEnum == False):
                    firstEnum = True
                else:
                    matFlags += " | "
                matFlags += "MAT_BACKFACE"
            if (app.renderCutout[numLines] == True):
                if (firstEnum == False):
                    firstEnum = True
                else:
                    matFlags += " | "
                matFlags += "MAT_INVISIBLE"
            if (app.renderFrontface[numLines] == True):
                if (firstEnum == False):
                    firstEnum = True
                else:
                    matFlags += " | "
                matFlags += "MAT_FRONTFACE"
            if (app.renderCI[numLines] == True):
                if (firstEnum == False):
                    firstEnum = True
                else:
                    matFlags += " | "
                matFlags += "MAT_CI"
            if (matFlags == ""):
                matFlags = "MAT_NULL"
            name += str(matFlags) + ", "
            name += str(app.materialCombiner[numLines]) + ", "
            colFlags = ""
            if (app.colGrip[numLines] == 0 and app.colIntangible[numLines] == False and app.colNocam[numLines] == False and app.colCamonly[numLines] == False):
                colFlags = "COLFLAG_NULL"
            else:
                firstEnum = False
                if (app.colSound[numLines] != "None"):
                    i = 0
                    while (i < len(app.materialSoundStrings)):
                        if (str(app.materialSoundStrings[i]) == str(app.colSound[numLines])):
                            colFlags = app.materialSoundEnums[i]
                            firstEnum = True
                            break
                        i+=1
                if (firstEnum == True):
                    colFlags += " | "
                else:
                    firstEnum = True
                colFlags += "COLFLAG_GRIP(" + str(app.colGrip[numLines]) + ")"
                if (app.colIntangible[numLines] == True):
                    colFlags += " | COLFLAG_INTANGIBLE"
                if (app.colCamonly[numLines] == True):
                    colFlags += " | COLFLAG_CAM_ONLY"
                if (app.colNocam[numLines] == True):
                    colFlags += " | COLFLAG_NO_CAM"
                if (app.colShadow[numLines] == True):
                    colFlags += " | COLFLAG_SHADOW"
            name += colFlags + ", "
            name += app.materialShiftS0[numLines] + ", "
            name += app.materialShiftT0[numLines] + ", "
            name += app.materialShiftS1[numLines] + ", "
            name += app.materialShiftT1[numLines] + ", "
            name += app.materialMoveS0[numLines] + ", "
            name += app.materialMoveT0[numLines] + ", "
            name += app.materialMoveS1[numLines] + ", "
            name += app.materialMoveT1[numLines]
            name += "}, // " + str(app.materialNames[numLines]) + "\n"
            lines.insert(6, name)
            new = "".join(name)
            fp.write(new)
            numLines += 1
        fp.write("};")
        write_enums()
        print("Saved materials!")

def refresh_textures():
    window.matTex0.clear()
    window.matTex1.clear()
    window.matTex0.addItems(["None"])
    window.matTex1.addItems(["None"])
    window.matTex0.addItems(app.textureNames)
    window.matTex1.addItems(app.textureNames)

def add_texture():
    window.texList.clear()
    index = app.textureCount
    app.textureCount += 1
    name = "texture" + str(index)
    enum = "TEXTURE_" + name.upper()
    app.textureNames.insert(index, name)
    app.textureEnums.insert(index, enum)
    app.textureClampH.insert(index, 0)
    app.textureClampV.insert(index, 0)
    app.textureMirrorH.insert(index, 0)
    app.textureMirrorV.insert(index, 0)
    app.textureFlipbookFrames.insert(index, 0)
    app.textureFlipbookSpeed.insert(index, 0)
    app.textureCategory.insert(index, "idk???")
    window.texList.addItems(app.textureNames)
    refresh_textures()

def add_material():
    window.matList.clear()
    index = app.materialCount
    app.materialCount += 1
    name = "material" + str(index)
    enum = "MATERIAL_" + name.upper()
    app.materialNames.insert(index, name)
    app.materialEnums.insert(index, enum)
    app.materialShiftS0.insert(index, "0")
    app.materialShiftS1.insert(index, "0")
    app.materialShiftT0.insert(index, "0")
    app.materialShiftT1.insert(index, "0")
    app.materialMoveS0.insert(index, "0")
    app.materialMoveS1.insert(index, "0")
    app.materialMoveT0.insert(index, "0")
    app.materialMoveT1.insert(index, "0")

    app.renderCutout.insert(index, False)
    app.renderXlu.insert(index, False)
    app.renderLighting.insert(index, False)
    app.renderFog.insert(index, True)
    app.renderEnvmap.insert(index, False)
    app.renderDepth.insert(index, True)
    app.renderVtxcol.insert(index, True)
    app.renderDecal.insert(index, False)
    app.renderInter.insert(index, False)
    app.renderBackface.insert(index, False)
    app.renderInvis.insert(index, False)
    app.renderFrontface.insert(index, False)
    app.renderCI.insert(index, False)

    app.colIntangible.insert(index, False)
    app.colNocam.insert(index, False)
    app.colCamonly.insert(index, False)
    app.colShadow.insert(index, False)
    app.colGrip.insert(index, 8)
    app.colSound.insert(index, "None")
    app.materialCombiner.insert(index, app.combinerNames[0])
    app.materialTex0.insert(index, "None")
    app.materialTex1.insert(index, "None")

    window.matList.addItems(app.materialNames)
    refresh_textures()

def delete_texture():
    if (not window.texList.currentRow() == -1):
        rowNum = app.texSelection
        app.textureNames.pop(rowNum)
        app.textureClampH.pop(rowNum)
        app.textureClampV.pop(rowNum)
        app.textureMirrorH.pop(rowNum)
        app.textureMirrorV.pop(rowNum)
        app.textureFlipbookFrames.pop(rowNum)
        app.textureFlipbookSpeed.pop(rowNum)
        app.textureEnums.pop(rowNum)
        window.texList.clear()
        window.texList.addItems(app.textureNames)
        write_textures()
        refresh_textures()


def delete_material():
    if (not window.matList.currentRow() == -1):
        rowNum = app.matSelection
        
        app.materialNames.pop(rowNum)
        app.materialEnums.pop(rowNum)
        app.materialShiftS0.pop(rowNum)
        app.materialShiftS1.pop(rowNum)
        app.materialShiftT0.pop(rowNum)
        app.materialShiftT1.pop(rowNum)
        app.materialMoveS0.pop(rowNum)
        app.materialMoveS1.pop(rowNum)
        app.materialMoveT0.pop(rowNum)
        app.materialMoveT1.pop(rowNum)

        app.renderCutout.pop(rowNum)
        app.renderXlu.pop(rowNum)
        app.renderLighting.pop(rowNum)
        app.renderFog.pop(rowNum)
        app.renderEnvmap.pop(rowNum)
        app.renderDepth.pop(rowNum)
        app.renderVtxcol.pop(rowNum)
        app.renderDecal.pop(rowNum)
        app.renderInter.pop(rowNum)
        app.renderBackface.pop(rowNum)
        app.renderInvis.pop(rowNum)
        app.renderFrontface.pop(rowNum)
        app.renderCI.pop(rowNum)

        app.colIntangible.pop(rowNum)
        app.colNocam.pop(rowNum)
        app.colCamonly.pop(rowNum)
        app.colShadow.pop(rowNum)
        app.colGrip.pop(rowNum)
        app.colSound.pop(rowNum)
        app.materialCombiner.pop(rowNum)
        app.materialTex0.pop(rowNum)
        app.materialTex1.pop(rowNum)
        window.matList.clear()
        window.matList.addItems(app.materialNames)
        write_materials()
        refresh_textures()

def set_active_texture():
    rowNum = window.texList.currentRow()
    app.texSelection = rowNum
    window.clampH.setChecked(app.textureClampH[rowNum])
    window.clampV.setChecked(app.textureClampV[rowNum])
    window.mirrorH.setChecked(app.textureMirrorH[rowNum])
    window.mirrorV.setChecked(app.textureMirrorV[rowNum])
    window.texFlipbookName.setText(str(app.textureFlipbookFrames[rowNum]))
    window.texFlipSpeedName.setText(str(app.textureFlipbookSpeed[rowNum]))
    window.texName.clear()
    window.texName.setText(str(app.textureNames[rowNum]))
    window.texCategory.setText("Folder: " + str(app.textureCategory[rowNum]))
    tex = QPixmap(app.rootDir + "/assets/textures/" + str(app.textureCategory[rowNum]) + "/" + app.textureNames[rowNum] + ".png")
    window.texImage.setPixmap(tex)
    window.texImage.resize(tex.width(), tex.height())

def set_active_material():
    rowNum = window.matList.currentRow()
    app.matSelection = rowNum
    window.matNameInput.setText(app.materialNames[rowNum])
    window.matShiftS0.setText(app.materialShiftS0[rowNum])
    window.matShiftS1.setText(app.materialShiftS1[rowNum])
    window.matShiftT0.setText(app.materialShiftT0[rowNum])
    window.matShiftT1.setText(app.materialShiftT1[rowNum])
    window.matScrollS0.setText(app.materialMoveS0[rowNum])
    window.matScrollS1.setText(app.materialMoveS1[rowNum])
    window.matScrollT0.setText(app.materialMoveT0[rowNum])
    window.matScrollT1.setText(app.materialMoveT1[rowNum])

    window.matFlagCutout.setChecked(app.renderCutout[rowNum])
    window.matFlagXlu.setChecked(app.renderXlu[rowNum])
    window.matFlagLighting.setChecked(app.renderLighting[rowNum])
    window.matFlagFog.setChecked(app.renderFog[rowNum])
    window.matFlagEnvmap.setChecked(app.renderEnvmap[rowNum])
    window.matFlagZBuffer.setChecked(app.renderDepth[rowNum])
    window.matFlagVtxCol.setChecked(app.renderVtxcol[rowNum])
    window.matFlagDecal.setChecked(app.renderDecal[rowNum])
    window.matFlagInter.setChecked(app.renderInter[rowNum])
    window.matFlagBackface.setChecked(app.renderBackface[rowNum])
    window.matFlagInvis.setChecked(app.renderInvis[rowNum])
    window.matFlagFrontface.setChecked(app.renderFrontface[rowNum])
    window.matFlagCI.setChecked(app.renderCI[rowNum])

    window.matColIntangible.setChecked(app.colIntangible[rowNum])
    window.matColNocam.setChecked(app.colNocam[rowNum])
    window.matColCamonly.setChecked(app.colCamonly[rowNum])
    window.matColShadow.setChecked(app.colShadow[rowNum])
    window.matColGripInput.setValue(int(app.colGrip[rowNum]))
    i = 0
    while (i < len(app.combinerNames)):
        if (str(app.combinerNames[i]) == str(app.materialCombiner[rowNum])):
            window.matCombiner.setCurrentIndex(i)
            break
        i+=1
    i = 0
    window.matTex0.setCurrentIndex(0)
    while (i < len(app.textureEnums)):
        if (app.textureEnums[i] == app.materialTex0[rowNum]):
            window.matTex0.setCurrentIndex(i + 1)
            break
        i += 1
    i = 0
    window.matTex1.setCurrentIndex(0)
    while (i < len(app.textureEnums)):
        if (app.textureEnums[i] == app.materialTex1[rowNum]):
            window.matTex1.setCurrentIndex(i + 1)
            break
        i += 1
    i = 0
    window.matColSoundInput.setCurrentIndex(0)
    while (i < len(app.materialSoundStrings)):
        if (app.materialSoundStrings[i] == app.colSound[rowNum]):
            window.matColSoundInput.setCurrentIndex(i)
            break
        i += 1

def save_texture():
    rowNum = app.texSelection
    app.textureClampH[rowNum] = window.clampH.isChecked()
    app.textureClampV[rowNum] = window.clampV.isChecked()
    app.textureMirrorH[rowNum] = window.mirrorH.isChecked()
    app.textureMirrorV[rowNum] = window.mirrorV.isChecked()
    app.textureNames[rowNum] = window.texName.text()
    app.textureFlipbookFrames[rowNum] = window.texFlipbookName.text()
    app.textureFlipbookSpeed[rowNum] = window.texFlipSpeedName.text()
    enumName = "TEXTURE_" + app.textureNames[rowNum].upper()
    enumName = enumName.split(".", 1)[0]
    app.textureEnums[rowNum] = enumName
    window.texList.clear()
    window.texList.addItems(app.textureNames)
    write_textures()

def save_material():
    rowNum = app.matSelection
    
    app.materialNames[rowNum] = window.matNameInput.text()
    app.materialShiftS0[rowNum] = window.matShiftS0.text()
    app.materialShiftS1[rowNum] = window.matShiftS1.text()
    app.materialShiftT0[rowNum] = window.matShiftT0.text()
    app.materialShiftT1[rowNum] = window.matShiftT1.text()
    app.materialMoveS0[rowNum] = window.matScrollS0.text()
    app.materialMoveS1[rowNum] = window.matScrollS1.text()
    app.materialMoveT0[rowNum] = window.matScrollT0.text()
    app.materialMoveT1[rowNum] = window.matScrollT1.text()

    app.renderCutout[rowNum] = window.matFlagCutout.isChecked()
    app.renderXlu[rowNum] = window.matFlagXlu.isChecked()
    app.renderLighting[rowNum] = window.matFlagLighting.isChecked()
    app.renderFog[rowNum] = window.matFlagFog.isChecked()
    app.renderEnvmap[rowNum] = window.matFlagEnvmap.isChecked()
    app.renderDepth[rowNum] = window.matFlagZBuffer.isChecked()
    app.renderVtxcol[rowNum] = window.matFlagVtxCol.isChecked()
    app.renderDecal[rowNum] = window.matFlagDecal.isChecked()
    app.renderInter[rowNum] = window.matFlagInter.isChecked()
    app.renderBackface[rowNum] = window.matFlagBackface.isChecked()
    app.renderInvis[rowNum] = window.matFlagInvis.isChecked()
    app.renderFrontface[rowNum] = window.matFlagFrontface.isChecked()
    app.renderCI[rowNum] = window.matFlagCI.isChecked()

    app.colIntangible[rowNum] = window.matColIntangible.isChecked()
    app.colNocam[rowNum] = window.matColNocam.isChecked()
    app.colCamonly[rowNum] = window.matColCamonly.isChecked()
    app.colShadow[rowNum] = window.matColShadow.isChecked()
    app.colGrip[rowNum] = window.matColGripInput.value()
    app.colSound[rowNum] = app.materialSoundStrings[window.matColSoundInput.currentIndex()]
    app.materialCombiner[rowNum] = app.combinerNames[window.matCombiner.currentIndex()]
    if (window.matTex0.currentIndex() != 0):
        app.materialTex0[rowNum] = app.textureEnums[window.matTex0.currentIndex() - 1]
    else:
        app.materialTex0[rowNum] = "-1"
    if (window.matTex1.currentIndex() != 0):
        app.materialTex1[rowNum] = app.textureEnums[window.matTex1.currentIndex() - 1]
    else:
        app.materialTex1[rowNum] = "-1"
    write_materials()

def update_grip(value):
    window.matColGripLabel.setText("Grip: " + str(value))

boot_config()
if check_valid_directory() is True:
    print("Starting.")
    make_window()
    init_tex_list()
    init_mat_list()
    window.saveTexButton.clicked.connect(save_texture)
    window.newTexButton.clicked.connect(add_texture)
    window.deleteTexButton.clicked.connect(delete_texture)
    window.newMatButton.clicked.connect(add_material)
    window.saveMatButton.clicked.connect(save_material)
    window.deleteMatButton.clicked.connect(delete_material)
    window.texList.itemClicked.connect(set_active_texture)
    window.matList.itemClicked.connect(set_active_material)
    window.texManagerButton.clicked.connect(switch_window_tex)
    window.matManagerButton.clicked.connect(switch_window_mat)
    window.matColGripInput.valueChanged.connect(update_grip)
    window.texList.setCurrentRow(0)
    window.matList.setCurrentRow(0)
    set_active_texture()
    refresh_textures()
    set_active_material()
    sys.exit(app.exec_())