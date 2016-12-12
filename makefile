include ..\makefile.in

TARGET = spiplaystream.exe
FLAGS += -mwindows
LIBS = -lcomdlg32

all: $(TARGET)

clean:
	$(RM) $(OUTDIR)\$(TARGET)
