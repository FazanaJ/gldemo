from PyQt5 import QtWidgets, QtCore
from PyQt5.QtWidgets import QApplication, QWidget, QMainWindow, QListWidget, QListWidgetItem, QLabel, QFileDialog
from PyQt5.QtGui import QPixmap
import configparser
import sys
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
    window.texList.resize(140, window.frameGeometry().height() - 22 - 80)
    window.saveTexButton.move(2, window.frameGeometry().height() - 20 - 80)
    window.newTexButton.move(2, window.frameGeometry().height() - 20 - 56)
    window.deleteTexButton.move(2, window.frameGeometry().height() - 20 - 32)
    
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
app.elementY = 0
app.textureNames = []
app.textureEnums = []
app.textureClampH = []
app.textureClampV = []
app.textureMirrorH = []
app.textureMirrorV = []
app.textureCutout = []
app.textureXlu = []
app.textureLighting = []
app.textureFog = []
app.textureEnvmap = []
app.textureDepth = []
app.textureVtxcol = []
app.textureDecal = []
app.textureInter = []
app.textureBackface = []
app.textureInvis = []
app.textureIntangible = []
app.textureCamOnly = []
app.textureNoCam = []
app.textureFrontface = []
app.textureCategory = []
app.textureCount = 0
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

def make_window():
    window.setWindowTitle("Texture Manager")
    window.texList = QListWidget(window)
    window.saveTexButton = create_button(window, 2, 32, 140, 24, "Save Texture", False)
    window.newTexButton = create_button(window, 2, 32, 140, 24, "Add Texture", False)
    window.deleteTexButton = create_button(window, 2, 32, 140, 24, "Delete Texture", False)
    window.show()
    window.saveTexButton.setVisible(True)
    window.newTexButton.setVisible(True)
    window.deleteTexButton.setVisible(True)
    app.elementY = 24
    window.texNameInput = create_label(window, 160, app.elementY, 160, 16, "Texture Name", True)
    window.texName = create_input(window, 160, app.elementY, 160, 28, True)

    window.clampH = create_tickbox(window, 160, app.elementY, 160, 16, "Clamp H", True)
    window.clampV = create_tickbox(window, 160, app.elementY, 160, 16, "Clamp V", True)
    app.elementY = 72
    window.mirrorH = create_tickbox(window, 240, app.elementY, 160, 16, "Mirror H", True)
    window.mirrorV = create_tickbox(window, 240, app.elementY, 160, 16, "Mirror V", True)

    app.elementY = 120
    window.cutout = create_tickbox(window, 160, app.elementY, 160, 16, "Cutout", True)
    window.xlu = create_tickbox(window, 160, app.elementY, 160, 16, "Semitransparent", True)
    window.lighting = create_tickbox(window, 160, app.elementY, 160, 16, "Lighting", True)
    window.fog = create_tickbox(window, 160, app.elementY, 160, 16, "Fog", True)
    window.envmap = create_tickbox(window, 160, app.elementY, 160, 16, "Env Mapping", True)
    window.depth = create_tickbox(window, 160, app.elementY, 160, 16, "Depth Buffer", True)
    window.vtxcol = create_tickbox(window, 160, app.elementY, 160, 16, "Vertex Colours", True)
    window.decal = create_tickbox(window, 160, app.elementY, 160, 16, "Decal", True)
    window.inter = create_tickbox(window, 160, app.elementY, 160, 16, "Interpenetrating", True)
    window.backface = create_tickbox(window, 160, app.elementY, 160, 16, "Enable Backface", True)
    window.invis = create_tickbox(window, 160, app.elementY, 160, 16, "Hidden Geometry", True)
    window.intangible = create_tickbox(window, 160, app.elementY, 160, 16, "Disable Collision", True)
    window.camonly = create_tickbox(window, 160, app.elementY, 160, 16, "Camera Collision", True)
    window.nocam = create_tickbox(window, 160, app.elementY, 160, 16, "Camera Passthrough", True)
    window.frontface = create_tickbox(window, 160, app.elementY, 160, 16, "Enable Frontface", True)

    window.texImage = QtWidgets.QLabel(window)
    window.texImage.move(160, app.elementY)
    window.texImage.setVisible(True)

def init_tex_list():
    file = open(app.rootDir + "/include/texture_Table.h")
    levelString = file.readlines()
    file.close()
    enumFound = False
    for line in levelString:
        if enumFound == False:
            ln2 = line.find("gTextureIDs")
            if not ln2 == -1:
                enumFound = True
        else:
            ln = line.find("};")
            ln3 = line.rfind('"')
            ln4 = line.find('"')
            if ln == -1:
                name = line[ln + 3:ln3]
                flagStr = line.partition(",")[2]
                flags = flagStr[ln + 2:-3]
                addClampH = False
                addClampV = False
                addMirrorH = False
                addMirrorV = False
                addCutout = False
                addXlu = False
                addLighting = False
                addFog = False
                addEnvmap = False
                addDepth = False
                addVtxcol = False
                addDecal = False
                addInter = False
                addBackface = False
                addInvis = False
                addIntangible = False
                addCamOnly = False
                addNoCam = False
                addFrontface = False
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
                if (not flags.find("MATERIAL_CUTOUT") == -1):
                    addCutout = True
                    useNum = False
                if (not flags.find("MATERIAL_XLU") == -1):
                    addXlu = True
                    useNum = False
                if (not flags.find("MATERIAL_LIGHTING") == -1):
                    addLighting = True
                    useNum = False
                if (not flags.find("MATERIAL_FOG") == -1):
                    addFog = True
                    useNum = False
                if (not flags.find("MATERIAL_ENVMAP") == -1):
                    addEnvmap = True
                    useNum = False
                if (not flags.find("MATERIAL_DEPTH_READ") == -1):
                    addDepth = True
                    useNum = False
                if (not flags.find("MATERIAL_VTXCOL") == -1):
                    addVtxcol = True
                    useNum = False
                if (not flags.find("MATERIAL_DECAL") == -1):
                    addDecal = True
                    useNum = False
                if (not flags.find("MATERIAL_INTER") == -1):
                    addInter = True
                    useNum = False
                if (not flags.find("MATERIAL_BACKFACE") == -1):
                    addBackface = True
                    useNum = False
                if (not flags.find("MATERIAL_INVISIBLE") == -1):
                    addInvis = True
                    useNum = False
                if (not flags.find("MATERIAL_INTANGIBLE") == -1):
                    addIntangible = True
                    useNum = False
                if (not flags.find("MATERIAL_CAM_ONLY") == -1):
                    addCamOnly = True
                    useNum = False
                if (not flags.find("MATERIAL_NO_CAM") == -1):
                    addNoCam = True
                    useNum = False
                if (not flags.find("MATERIAL_FRONTFACE") == -1):
                    addFrontface = True
                    useNum = False
                if useNum == True:
                    num = int(flags)
                    if (num >= 262144):
                        num -= 262144
                        addFrontface = True
                    if (num >= 131072):
                        num -= 131072
                        addNoCam = True
                    if (num >= 65536):
                        num -= 65536
                        addCamOnly = True
                    if (num >= 32768):
                        num -= 32768
                        addIntangible = True
                    if (num >= 16384):
                        num -= 16384
                        addInvis = True
                    if (num >= 8192):
                        num -= 8192
                        addBackface = True
                    if (num >= 4096):
                        num -= 4096
                        addInter = True
                    if (num >= 2048):
                        num -= 2048
                        addDecal = True
                    if (num >= 1024):
                        num -= 1024
                        addVtxcol = True
                    if (num >= 512):
                        num -= 512
                        addDepth = True
                    if (num >= 256):
                        num -= 256
                        addEnvmap = True
                    if (num >= 128):
                        num -= 128
                        addFog = True
                    if (num >= 64):
                        num -= 64
                        addLighting = True
                    if (num >= 32):
                        num -= 32
                        addXlu = True
                    if (num >= 16):
                        num -= 16
                        addCutout = True
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
                app.textureClampH.insert(app.textureCount, addClampH)
                app.textureClampV.insert(app.textureCount, addClampV)
                app.textureMirrorH.insert(app.textureCount, addMirrorH)
                app.textureMirrorV.insert(app.textureCount, addMirrorV)
                app.textureCutout.insert(app.textureCount, addCutout)
                app.textureXlu.insert(app.textureCount, addXlu)
                app.textureLighting.insert(app.textureCount, addLighting)
                app.textureFog.insert(app.textureCount, addFog)
                app.textureEnvmap.insert(app.textureCount, addEnvmap)
                app.textureDepth.insert(app.textureCount, addDepth)
                app.textureVtxcol.insert(app.textureCount, addVtxcol)
                app.textureDecal.insert(app.textureCount, addDecal)
                app.textureInter.insert(app.textureCount, addInter)
                app.textureBackface.insert(app.textureCount, addBackface)
                app.textureInvis.insert(app.textureCount, addInvis)
                app.textureIntangible.insert(app.textureCount, addIntangible)
                app.textureCamOnly.insert(app.textureCount, addCamOnly)
                app.textureNoCam.insert(app.textureCount, addNoCam)
                app.textureFrontface.insert(app.textureCount, addFrontface)
                app.textureNames.insert(index, name)
                app.textureCount += 1
    enumFound = False
    file = open(app.rootDir + "/include/enums.h")
    levelString = file.readlines()
    file.close()
    for line in levelString:
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
    
    window.texList.addItems(app.textureNames)

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
            if (app.textureClampH[numLines] == True):
                totalFlags += 1
            if (app.textureClampV[numLines] == True):
                totalFlags += 2
            if (app.textureMirrorH[numLines] == True):
                totalFlags += 4
            if (app.textureMirrorV[numLines] == True):
                totalFlags += 8
            if (app.textureCutout[numLines] == True):
                totalFlags += 16
            if (app.textureXlu[numLines] == True):
                totalFlags += 32
            if (app.textureLighting[numLines] == True):
                totalFlags += 64
            if (app.textureFog[numLines] == True):
                totalFlags += 128
            if (app.textureEnvmap[numLines] == True):
                totalFlags += 256
            if (app.textureDepth[numLines] == True):
                totalFlags += 512
            if (app.textureVtxcol[numLines] == True):
                totalFlags += 1024
            if (app.textureDecal[numLines] == True):
                totalFlags += 2048
            if (app.textureInter[numLines] == True):
                totalFlags += 4096
            if (app.textureBackface[numLines] == True):
                totalFlags += 8192
            if (app.textureInvis[numLines] == True):
                totalFlags += 16384
            if (app.textureIntangible[numLines] == True):
                totalFlags += 32768
            if (app.textureCamOnly[numLines] == True):
                totalFlags += 65536
            if (app.textureNoCam[numLines] == True):
                totalFlags += 131072
            if (app.textureFrontface[numLines] == True):
                totalFlags += 262144
            name = '{"' + app.textureNames[numLines] + '", ' + str(totalFlags) + '},\n'
            lines.insert(6, name)
            new = "".join(name)
            fp.write(new)
            numLines += 1
        fp.write("};")
    
    with open(app.rootDir + "/include/enums.h", 'r+') as fp:
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
            if (not line.find("TextureNames") == -1):
                firstLine = line
            if (line.find(",") == -1 and line.find("};") == -1):
                fp.write(line)
            
        numLines = 0
        for i in app.textureNames:
            name = app.textureEnums[numLines] + ',\n'
            lines.insert(3, name)
            new = "".join(name)
            fp.write(new)
            numLines += 1
        fp.write("};")
    

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
    app.textureCutout.insert(index, 0)
    app.textureXlu.insert(index, 0)
    app.textureLighting.insert(index, 0)
    app.textureFog.insert(index, 0)
    app.textureEnvmap.insert(index, 0)
    app.textureDepth.insert(index, 0)
    app.textureVtxcol.insert(index, 0)
    app.textureDecal.insert(index, 0)
    app.textureInter.insert(index, 0)
    app.textureBackface.insert(index, 0)
    app.textureInvis.insert(index, 0)
    app.textureIntangible.insert(index, 0)
    app.textureCamOnly.insert(index, 0)
    app.textureNoCam.insert(index, 0)
    app.textureFrontface.insert(index, 0)
    window.texList.addItems(app.textureNames)

def delete_texture():
    if (not window.texList.currentRow() == -1):
        rowNum = window.texList.currentRow()
        app.textureNames.pop(rowNum)
        app.textureClampH.pop(rowNum)
        app.textureClampV.pop(rowNum)
        app.textureMirrorH.pop(rowNum)
        app.textureMirrorV.pop(rowNum)
        app.textureCutout.pop(rowNum)
        app.textureXlu.pop(rowNum)
        app.textureLighting.pop(rowNum)
        app.textureFog.pop(rowNum)
        app.textureEnvmap.pop(rowNum)
        app.textureDepth.pop(rowNum)
        app.textureVtxcol.pop(rowNum)
        app.textureDecal.pop(rowNum)
        app.textureInter.pop(rowNum)
        app.textureBackface.pop(rowNum)
        app.textureInvis.pop(rowNum)
        app.textureIntangible.pop(rowNum)
        app.textureCamOnly.pop(rowNum)
        app.textureNoCam.pop(rowNum)
        app.textureFrontface.pop(rowNum)
        app.textureEnums.pop(rowNum)
        window.texList.clear()
        window.texList.addItems(app.textureNames)
        write_textures()

def set_active_Texture():
    rowNum = window.texList.currentRow()
    window.clampH.setChecked(app.textureClampH[rowNum])
    window.clampV.setChecked(app.textureClampV[rowNum])
    window.mirrorH.setChecked(app.textureMirrorH[rowNum])
    window.mirrorV.setChecked(app.textureMirrorV[rowNum])
    window.cutout.setChecked(app.textureCutout[rowNum])
    window.xlu.setChecked(app.textureXlu[rowNum])
    window.lighting.setChecked(app.textureLighting[rowNum])
    window.fog.setChecked(app.textureFog[rowNum])
    window.envmap.setChecked(app.textureEnvmap[rowNum])
    window.depth.setChecked(app.textureDepth[rowNum])
    window.vtxcol.setChecked(app.textureVtxcol[rowNum])
    window.decal.setChecked(app.textureDecal[rowNum])
    window.inter.setChecked(app.textureInter[rowNum])
    window.backface.setChecked(app.textureBackface[rowNum])
    window.invis.setChecked(app.textureInvis[rowNum])
    window.intangible.setChecked(app.textureIntangible[rowNum])
    window.camonly.setChecked(app.textureCamOnly[rowNum])
    window.nocam.setChecked(app.textureNoCam[rowNum])
    window.frontface.setChecked(app.textureFrontface[rowNum])
    window.texName.clear()
    window.texName.setText(str(app.textureNames[rowNum]))
    tex = QPixmap("../assets/textures/" + app.textureNames[rowNum] + ".png")
    window.texImage.setPixmap(tex)
    window.texImage.resize(tex.width(), tex.height())

def save_texture():
    rowNum = window.texList.currentRow()
    app.textureClampH[rowNum] = window.clampH.isChecked()
    app.textureClampV[rowNum] = window.clampV.isChecked()
    app.textureMirrorH[rowNum] = window.mirrorH.isChecked()
    app.textureMirrorV[rowNum] = window.mirrorV.isChecked()
    app.textureCutout[rowNum] = window.cutout.isChecked()
    app.textureXlu[rowNum] = window.xlu.isChecked()
    app.textureLighting[rowNum] = window.lighting.isChecked()
    app.textureFog[rowNum] = window.fog.isChecked()
    app.textureEnvmap[rowNum] = window.envmap.isChecked()
    app.textureDepth[rowNum] = window.depth.isChecked()
    app.textureVtxcol[rowNum] = window.vtxcol.isChecked()
    app.textureDecal[rowNum] = window.decal.isChecked()
    app.textureInter[rowNum] = window.inter.isChecked()
    app.textureBackface[rowNum] = window.backface.isChecked()
    app.textureInvis[rowNum] = window.invis.isChecked()
    app.textureIntangible[rowNum] = window.intangible.isChecked()
    app.textureCamOnly[rowNum] = window.camonly.isChecked()
    app.textureNoCam[rowNum] = window.nocam.isChecked()
    app.textureFrontface[rowNum] = window.frontface.isChecked()
    app.textureNames[rowNum] = window.texName.text()
    enumName = "TEXTURE_" + app.textureNames[rowNum].upper()
    enumName = enumName.split(".", 1)[0]
    app.textureEnums[rowNum] = enumName
    window.texList.clear()
    window.texList.addItems(app.textureNames)
    write_textures()

boot_config()
if check_valid_directory() is True:
    print("Starting.")
    make_window()
    init_tex_list()
    window.saveTexButton.clicked.connect(save_texture)
    window.newTexButton.clicked.connect(add_texture)
    window.deleteTexButton.clicked.connect(delete_texture)
    window.texList.itemClicked.connect(set_active_Texture)
    sys.exit(app.exec_())