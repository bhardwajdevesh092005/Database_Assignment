import matplotlib.pyplot as plt
import numpy as np

# --- Data from the report ---
# This data is hardcoded from the test results for simplicity.
labels = ['10%', '20%', '30%', '40%', '50%', '60%', '70%', '80%', '90%']

# LRU Data
lru_phys_reads = [672.00, 674.00, 675.40, 665.40, 664.80, 663.40, 668.00, 663.00, 678.80]
lru_phys_writes = [621.80, 576.60, 531.60, 470.20, 401.80, 344.40, 271.00, 199.00, 109.00]
lru_hit_rate = [32.80, 32.60, 32.46, 33.46, 33.52, 33.66, 33.20, 33.70, 32.12]

# MRU Data
mru_phys_reads = [675.00, 677.60, 666.20, 670.00, 674.00, 670.40, 666.20, 665.00, 669.40]
mru_phys_writes = [628.40, 572.20, 520.60, 465.00, 403.20, 335.60, 263.80, 189.40, 111.80]
mru_hit_rate = [32.50, 32.24, 33.38, 33.00, 32.60, 32.96, 33.38, 33.50, 33.06]

# --- Plotting ---
x = np.arange(len(labels))  # the label locations
width = 0.2  # the width of the bars

fig, ax1 = plt.subplots(figsize=(16, 8))

# --- Bar Chart for I/O Counts ---
rects1 = ax1.bar(x - 1.5*width, lru_phys_reads, width, label='LRU Phys. Reads', color='cornflowerblue')
rects2 = ax1.bar(x - 0.5*width, mru_phys_reads, width, label='MRU Phys. Reads', color='lightsteelblue')
rects3 = ax1.bar(x + 0.5*width, lru_phys_writes, width, label='LRU Phys. Writes', color='salmon')
rects4 = ax1.bar(x + 1.5*width, mru_phys_writes, width, label='MRU Phys. Writes', color='lightcoral')

# Add some text for labels, title and axes ticks
ax1.set_xlabel('Read Percentage in Workload', fontweight='bold')
ax1.set_ylabel('Average Physical I/O Count', fontweight='bold')
ax1.set_title('Performance of LRU vs. MRU under Randomized Workloads', fontweight='bold', fontsize=16)
ax1.set_xticks(x)
ax1.set_xticklabels(labels)
ax1.grid(axis='y', linestyle='--', alpha=0.7)

# --- Line Chart for Hit Rate (on a secondary y-axis) ---
ax2 = ax1.twinx()  # instantiate a second axes that shares the same x-axis
ax2.set_ylabel('Average Buffer Hit Rate (%)', fontweight='bold', color='green')
line1, = ax2.plot(x, lru_hit_rate, 'o-', color='darkgreen', label='LRU Hit Rate')
line2, = ax2.plot(x, mru_hit_rate, 'x-', color='limegreen', label='MRU Hit Rate')
ax2.tick_params(axis='y', labelcolor='green')
ax2.set_ylim(0, 100)

# --- Legend ---
# Combine legends from both axes
bars = [rects1, rects2, rects3, rects4]
lines = [line1, line2]
ax1.legend(bars + lines, [b.get_label() for b in bars] + [l.get_label() for l in lines], loc='upper center')


fig.tight_layout()  # otherwise the right y-label is slightly clipped

# --- Save the figure ---
output_filename = 'performance_graph.png'
plt.savefig(output_filename)

print(f"Graph successfully generated and saved as '{output_filename}'")
