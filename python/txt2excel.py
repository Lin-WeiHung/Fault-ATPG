import re
from pathlib import Path
import pandas as pd
import sys

if len(sys.argv) != 3:
    print("❌ 使用方式: python txt2excel.py <輸入檔案> <輸出檔案>")
    sys.exit(1)

TXT_PATH = Path(sys.argv[1])  # 外部輸入的 TXT 檔案路徑
EXCEL_OUT = Path(sys.argv[2])  # 外部輸入的 Excel 檔案路徑

# ----------------------------------------------------------------------------------
# 1) 事先編譯正規表示式
fault_re   = re.compile(r'^\s*(.+?Fault.*)$')
subcase_re = re.compile(r'^Subcase\s+(\d+)\s+<\s*([^>]+)\s*>')
init_re    = re.compile(
    r'^Init\s+([01]):\s+([01]+|No detection)(?:\s+\((0x[0-9a-fA-F]+)\))?'
)

# ----------------------------------------------------------------------------------
# 2) 逐行掃描
rows_0, rows_1 = [], []
cur_fault = cur_subcase = sfr_tag = None

with TXT_PATH.open("r", encoding="utf-8") as f:
    lines = f.readlines()

i, n = 0, len(lines)
while i < n:
    line = lines[i].rstrip("\n")

    # 2-1 Fault 標頭
    m_fault = fault_re.match(line)
    if m_fault:
        cur_fault = m_fault.group(1).strip()
        i += 1
        continue

    # 2-2 Subcase
    m_sub = subcase_re.match(line)
    if m_sub:
        cur_subcase = int(m_sub.group(1))
        sfr_tag     = m_sub.group(2).strip()      # 直接保留整段字串
        i += 1
        continue

    # 2-3 Init 0 / Init 1
    m_init = init_re.match(line)
    if m_init:
        init_id       = int(m_init.group(1))             # 0 或 1
        syndrome_bin  = m_init.group(2)                  # 可能為 "No detection"
        hex_code      = m_init.group(3) or ""            # 可能為空字串

        # 嘗試讀取下一行的偵測結果
        detect_ops = ""
        if i + 1 < n:
            nxt = lines[i + 1].lstrip()
            # 若下一行不是新段落 (Subcase/Fault/Init) 且非空白，視為偵測清單
            if (nxt and
                not nxt.startswith(("Init", "Subcase", "dynamic", "Stuck",
                                   "Transition", "Write", "Read", "Disturb",
                                   "Incorrect", "State", "Detected Rate"))):
                detect_ops = nxt.rstrip("\n")
                i += 1  # 額外跳過此行

        row = {
            "Fault Name"            : cur_fault,
            "Subcase <idx>"         : cur_subcase,
            "<S / F / R>"           : sfr_tag,
            f"Init {init_id} <syndrome>"      : "" if syndrome_bin == "No detection" else syndrome_bin,
            f"Init {init_id} <hex_syndrome>" : hex_code,
            f"Init {init_id} <detect OPs>"   : detect_ops if syndrome_bin != "No detection" else "No detection"
        }

        # 放進對應 list
        if init_id == 0:
            rows_0.append(row)
        else:
            rows_1.append(row)

    i += 1

# ----------------------------------------------------------------------------------
# 3) 轉成 DataFrame，並確保欄位順序
cols0 = ["Fault Name", "Subcase <idx>", "<S / F / R>",
         "Init 0 <syndrome>", "Init 0 <hex_syndrome>", "Init 0 <detect OPs>"]
cols1 = ["Fault Name", "Subcase <idx>", "<S / F / R>",
         "Init 1 <syndrome>", "Init 1 <hex_syndrome>", "Init 1 <detect OPs>"]

df0 = pd.DataFrame(rows_0)[cols0]
df1 = pd.DataFrame(rows_1)[cols1]

# ----------------------------------------------------------------------------------
# 4) 匯出成 Excel：兩張分頁
with pd.ExcelWriter(EXCEL_OUT, engine="openpyxl") as writer:
    df0.to_excel(writer, sheet_name="Init0", index=False)
    df1.to_excel(writer, sheet_name="Init1", index=False)

print(f"✅  解析完成，已匯出：{EXCEL_OUT}")
