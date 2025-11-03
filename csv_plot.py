
import pandas as pd
import matplotlib.pyplot as plt
from io import StringIO

with open('data.csv', 'r', encoding='utf-8') as f:
    lines = f.readlines()

def read_block(marker):
    marker = marker.lower()
    for i, line in enumerate(lines):
        if line.strip().lower().startswith(marker):
            block = []
            for ln in lines[i+1:]:
                if ln.strip() == '' or ln.strip().startswith('/'):
                    break
                block.append(ln)
            return pd.read_csv(StringIO(''.join(block)), skipinitialspace=True)
    raise ValueError(f"Marker '{marker}' not found in file")

df_fifo = read_block('/fifo')
df_lru  = read_block('/lru')

for df in (df_fifo, df_lru):
    df['Marcos'] = df['Marcos'].astype(int)
    df['Fallos'] = df['Fallos'].astype(int)

fig, axes = plt.subplots(1, 2, figsize=(10, 4), sharey=True)

axes[0].bar(df_fifo['Marcos'], df_fifo['Fallos'])
axes[0].set_title('FIFO')
axes[0].set_xlabel('Marcos')
axes[0].set_ylabel('Fallos')

axes[1].bar(df_lru['Marcos'], df_lru['Fallos'])
axes[1].set_title('LRU')
axes[1].set_xlabel('Marcos')

plt.tight_layout()
plt.show()


