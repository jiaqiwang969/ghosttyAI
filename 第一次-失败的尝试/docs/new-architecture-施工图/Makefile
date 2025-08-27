# PlantUML编译Makefile
# 用于批量将.puml文件编译为.svg文件

# 配置变量
PUML_FILES := $(wildcard *.puml)
SVG_FILES := $(PUML_FILES:.puml=.svg)
OUTPUT_DIR := uml-svg

# 默认目标
.PHONY: all clean help

# 默认目标：编译所有PlantUML文件
all: $(SVG_FILES)

# 创建输出目录
$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

# 编译规则：将.puml文件编译为.svg文件
%.svg: %.puml | $(OUTPUT_DIR)
	@echo "编译 $< 为 $@..."
	@plantuml -tsvg -o$(OUTPUT_DIR) $<
	@echo "完成: $(OUTPUT_DIR)/$@"

# 清理目标：删除所有生成的SVG文件
clean:
	@echo "清理生成的SVG文件..."
	@rm -rf $(OUTPUT_DIR)
	@echo "清理完成"

# 显示帮助信息
help:
	@echo "PlantUML编译Makefile使用说明："
	@echo ""
	@echo "可用目标："
	@echo "  all     - 编译所有.puml文件为.svg文件（默认）"
	@echo "  clean   - 清理所有生成的.svg文件"
	@echo "  help    - 显示此帮助信息"
	@echo ""
	@echo "使用方法："
	@echo "  make              # 编译所有文件"
	@echo "  make clean        # 清理生成的文件"
	@echo "  make help         # 显示帮助"
	@echo ""
	@echo "注意事项："
	@echo "  - 需要安装PlantUML"
	@echo "  - 生成的SVG文件将保存在 $(OUTPUT_DIR)/ 目录中"
	@echo "  - 如果输出目录不存在，将自动创建"

# 检查PlantUML是否安装
check-plantuml:
	@which plantuml > /dev/null || (echo "错误: 未找到PlantUML，请先安装"; exit 1)
	@echo "PlantUML已安装，版本信息："
	@plantuml -version

# 显示将要编译的文件列表
list:
	@echo "找到以下PlantUML文件："
	@for file in $(PUML_FILES); do echo "  $$file"; done
	@echo ""
	@echo "将生成以下SVG文件："
	@for file in $(SVG_FILES); do echo "  $(OUTPUT_DIR)/$$file"; done

# 安装依赖（macOS）
install-deps-macos:
	@echo "在macOS上安装PlantUML..."
	@brew install plantuml || (echo "请先安装Homebrew: https://brew.sh/"; exit 1)
	@echo "PlantUML安装完成"

# 安装依赖（Ubuntu/Debian）
install-deps-ubuntu:
	@echo "在Ubuntu/Debian上安装PlantUML..."
	@sudo apt-get update
	@sudo apt-get install -y plantuml
	@echo "PlantUML安装完成"

# 安装依赖（CentOS/RHEL）
install-deps-centos:
	@echo "在CentOS/RHEL上安装PlantUML..."
	@sudo yum install -y plantuml || sudo dnf install -y plantuml
	@echo "PlantUML安装完成"
