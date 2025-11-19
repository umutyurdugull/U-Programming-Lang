
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
LDFLAGS = -lm

TARGET = ulang
SOURCES = main.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@


run: $(TARGET)
ifndef SCRIPT
	@echo "Hata: Calistirilacak script dosyasini belirtmelisiniz. Ornek: make run SCRIPT=test.ul"
else
	@echo "--- ULang Script Calistiriliyor: $(SCRIPT) ---"
	./$(TARGET) $(SCRIPT)
endif

clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Temizlik tamamlandi. (*.o ve $(TARGET) silindi.)"

rebuild: clean all

.PHONY: all clean rebuild run