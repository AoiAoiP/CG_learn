# 编译器与编译选项
CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
INCLUDES = -I/usr/include -I../glad/include

# 链接库与库路径
LDFLAGS  = -L/usr/local/lib
LIBS     = -lglfw -lGLEW -lGL -lm -ldl -lassimp

# 最终生成的可执行文件名
TARGET = opengl_app

# 源文件与目标文件定义
# 自动查找当前目录下所有 .cpp 文件
SRCS = $(wildcard *.cpp)
# 添加GLAD源文件路径
GLAD_SRC = ../glad/src/glad.c
# 将所有 .cpp 文件名转换为对应的 .o 文件名
OBJS = $(SRCS:.cpp=.o) glad.o

# 默认目标：编译整个项目
all: $(TARGET)

# 链接目标：将 .o 文件链接为最终可执行文件
$(TARGET): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LIBS)

# 编译规则：将每个 .cpp 文件编译为 .o 文件
# 使用模式规则，$< 代表第一个依赖文件（.cpp），$@ 代表目标文件（.o）
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# 编译GLAD C源文件 - 需要不同的编译选项（C语言）
glad.o: $(GLAD_SRC)
	$(CXX) $(CXXFLAGS) -I../glad/include -c $(GLAD_SRC) -o glad.o

# 清理目标：删除编译生成的中间文件和可执行文件
clean:
	rm -f $(OBJS) $(TARGET) glad.0

# 检查GLAD文件是否存在
check-glad:
	@if [ ! -f "../glad/src/glad.c" ]; then \
		echo "错误：找不到GLAD源文件"; \
		echo "请确保GLAD安装在 ~/OpenGL/glad/ 目录下"; \
		echo "目录结构应为："; \
		echo "  ~/OpenGL/glad/include/glad/glad.h"; \
		echo "  ~/OpenGL/glad/src/glad.c"; \
		exit 1; \
	else \
		echo "✓ GLAD文件检查通过"; \
	fi
	@if [ ! -f "../glad/include/glad/glad.h" ]; then \
		echo "错误：找不到GLAD头文件"; \
		exit 1; \
	fi
