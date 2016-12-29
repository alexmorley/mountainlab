TEMPLATE = subdirs

# usage:
# qmake
# qmake "COMPONENTS = mountainview"

#COMPONENTS = mdaconvert  mountainprocess mountainview prv prv-gui

isEmpty(COMPONENTS) {
    COMPONENTS = mdaconvert mountainprocess mountainsort mountainview mountaincompare prv prv-gui
}

CONFIG += ordered

SUBDIRS += mlcommon/src/mlcommon.pro

defineReplace(ifcomponent) {
  contains(COMPONENTS, $$1) {
    message(Enabling $$1)
    return($$2)
  }
  return("")
}

SUBDIRS += $$ifcomponent(mdaconvert,mdaconvert/src/mdaconvert.pro)
SUBDIRS += $$ifcomponent(mountainbrowser,mountainbrowser/src/mountainbrowser.pro)
SUBDIRS += $$ifcomponent(mountainprocess,mountainprocess/src/mountainprocess.pro)
SUBDIRS += $$ifcomponent(mountainsort,mountainsort/src/mountainsort.pro)
SUBDIRS += $$ifcomponent(mountainview,mountainview/src/mountainview.pro)
SUBDIRS += $$ifcomponent(mountaincompare,mountaincompare/src/mountaincompare.pro)
SUBDIRS += $$ifcomponent(prv,prv/src/prv.pro)
SUBDIRS += $$ifcomponent(prv-gui,prv-gui/src/prv-gui.pro)
SUBDIRS += $$ifcomponent(mountainview-eeg,packages/mountainlab-eeg/mountainview-eeg/src/mountainview-eeg.pro)

CONFIG(debug, debug|release) { SUBDIRS += tests }


deb.target = deb
deb.commands = debuild $(DEBUILD_OPTS) -us -uc

QMAKE_EXTRA_TARGETS += deb