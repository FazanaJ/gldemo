from PyQt5 import QtWidgets, QtCore
from PyQt5.QtWidgets import QApplication, QWidget, QMainWindow, QListWidget, QListWidgetItem, QLabel
from PyQt5.QtGui import QPixmap
import sys
import os

def check_valid_directory():
    if (os.path.exists("../assets/textures")):
        print("Texture directory found.")
        return True
    else:
        print("Texture directory missing.")
        return False
    
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
app.textureCount = 0
window = Window()

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
    window.texName = create_input(window, 160, app.elementY, 100, 28, True)
    window.clampH = create_tickbox(window, 160, app.elementY, 160, 16, "Clamp H", True)
    window.clampV = create_tickbox(window, 160, app.elementY, 160, 16, "Clamp V", True)
    window.mirrorH = create_tickbox(window, 160, app.elementY, 160, 16, "Mirror H", True)
    window.mirrorV = create_tickbox(window, 160, app.elementY, 160, 16, "Mirror H", True)
    window.texImage = QtWidgets.QLabel(window)
    window.texImage.move(160, app.elementY)
    window.texImage.setVisible(True)

def init_tex_list():
    file = open("../include/texture_Table.h")
    levelString = file.readlines()
    file.close()
    enumFound = False
    for line in levelString:
        if enumFound == False:
            ln2 = line.find("sTextureIDs")
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
                app.textureClampH.insert(app.textureCount, addClampH)
                app.textureClampV.insert(app.textureCount, addClampV)
                app.textureMirrorH.insert(app.textureCount, addMirrorH)
                app.textureMirrorV.insert(app.textureCount, addMirrorV)
                app.textureNames.insert(index, name)
                app.textureCount += 1
    enumFound = False
    file = open("../include/enums.h")
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
                print(name)
                index = len(app.textureEnums)
                app.textureEnums.insert(index, name)
    
    window.texList.addItems(app.textureNames)

def write_textures():
    with open("../include/texture_table.h", 'r+') as fp:
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
            if (not line.find("sTextureIDs") == -1):
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
            name = '{"' + app.textureNames[numLines] + '", ' + str(totalFlags) + '},\n'
            lines.insert(6, name)
            new = "".join(name)
            fp.write(new)
            numLines += 1
        fp.write("};")
    
    with open("../include/enums.h", 'r+') as fp:
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
    window.texList.addItems(app.textureNames)

def delete_texture():
    if (not window.texList.currentRow() == -1):
        rowNum = window.texList.currentRow()
        app.textureNames.pop(rowNum)
        app.textureClampH.pop(rowNum)
        app.textureClampV.pop(rowNum)
        app.textureMirrorH.pop(rowNum)
        app.textureMirrorV.pop(rowNum)
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
    app.textureNames[rowNum] = window.texName.text()
    enumName = "TEXTURE_" + app.textureNames[rowNum].upper()
    enumName = enumName.split(".", 1)[0]
    app.textureEnums[rowNum] = enumName
    window.texList.clear()
    window.texList.addItems(app.textureNames)
    write_textures()

if check_valid_directory() is True:
    print("Starting.")
    make_window()
    init_tex_list()
    window.saveTexButton.clicked.connect(save_texture)
    window.newTexButton.clicked.connect(add_texture)
    window.deleteTexButton.clicked.connect(delete_texture)
    window.texList.itemClicked.connect(set_active_Texture)
    sys.exit(app.exec_())