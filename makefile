# ======== 參數 ========
CXX       := g++
CXXFLAGS  := -std=c++20 -O2 -Wall -Wextra -g
INCLUDES  := -Iinclude
LDFLAGS   :=    
OUT 	 := Fault_simulator

                                
# ======== 自動偵測 ========
SRC_DIR   := src
TEST_DIR  := test

SRC_CPP   := $(wildcard $(SRC_DIR)/*.cpp)
OBJS      := $(patsubst %.cpp,%.o,$(SRC_CPP))

# 從 tests/t_*.cpp 推導出「可測試模組名稱」
TEST_SRC   := $(wildcard $(TEST_DIR)/t_*.cpp)
TEST_NAMES := $(patsubst $(TEST_DIR)/t_%.cpp,%,$(TEST_SRC))

# 若沒有偵測到任何測試檔，NUMBERS 留空，避免 word() 出錯
ifneq ($(strip $(TEST_NAMES)),)
  # 把 seq 的換行轉成空白，同時去掉尾端換行
  NUMBERS := $(shell seq 1 $(words $(TEST_NAMES)) | tr '\n' ' ' | sed 's/ *$$//')
endif

# ======== 一般編譯 ========
# make com → 編譯整套程式
com: $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC_CPP) -o $(OUT) $(LDFLAGS)

# make run → 執行 Fault_simulator
# 需要提供 faults.json、marchTest.json 和輸出檔案名稱
FAULT = fault.json
MARCH = March-LSD.json
OUTPUTFILE = Detection_report.txt
run:
	./$(OUT) $(FAULT) $(MARCH) $(OUTPUTFILE)
	python3 txt2excel.py $(OUTPUTFILE) $(OUTPUTFILE:.txt=.xlsx)

make all: com run

# ======== 測試機制 ========
# make test → 列出所有可用測試並編號
test:
	@echo "===== 可執行測試列表 ====="
	@i=1; for name in $(TEST_NAMES); do \
		printf " %2d) %s\n" $$i $$name; \
		i=`expr $$i + 1`; \
	done

# 動態產生數字目標：make 1、make 2 ...
define MAKE_NUM_RULE
$(1): ; @$(MAKE) run-test TEST=$(word $(1),$(TEST_NAMES))
endef
$(foreach n,$(NUMBERS),$(eval $(call MAKE_NUM_RULE,$(n))))

# 真正編譯並以 gdb 執行單一測試
run-test:
	@echo ">> 正在編譯 ：$(TEST)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) \
			$(TEST_DIR)/t_$(TEST).cpp \
			-o $(TEST_DIR)/t_$(TEST) $(LDFLAGS)
	./$(TEST_DIR)/t_$(TEST)

# ======== 清理 ========
clean:
	$(RM) $(OBJS) $(TEST_DIR)/*[^.cpp] $(SRC_DIR)/*.o

.PHONY: com run test run-test clean

print-vars:
	@echo "SRC_DIR   = $(SRC_DIR)"
	@echo "SRC_CPP   = $(SRC_CPP)"
	@echo "OBJS      = $(OBJS)"
	@echo "TEST_DIR  = $(TEST_DIR)"
	@echo "TEST_SRC  = $(TEST_SRC)"
	@echo "TEST_NAMES= $(TEST_NAMES)"
	@echo "NUMBERS   = $(NUMBERS)"