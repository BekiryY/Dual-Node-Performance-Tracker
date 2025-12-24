import csv
import matplotlib.pyplot as plt

def generate_plot(filename):
    times = []
    intensities = []
    bpms = []
    temps = []
    
    current_time_ms = 0
    
    try:
        with open(filename, 'r', newline='') as f:
            reader = csv.DictReader(f)
            
            # Use strip() on keys just in case of whitespace in headers
            reader.fieldnames = [name.strip() for name in reader.fieldnames]
            
            for row in reader:
                try:
                    period = int(row['Period'])
                    intensity = int(row['Intensity'])
                    bpm_str = row['AvgBPM']
                    temp_str = row.get('Temperature', '0.0')
                    
                    # Handle potential empty strings or whitespace
                    if not bpm_str or bpm_str.strip() == '':
                        bpm = 0.0
                    else:
                        bpm = float(bpm_str)
                        
                    if not temp_str or temp_str.strip() == '':
                        temp = 0.0
                    else:
                        temp = float(temp_str)
                        
                except (ValueError, KeyError) as e:
                    # Skip rows that don't parse correctly or missing columns
                    continue

                # Filter out empty/invalid steps as per original logic
                if period == 0 and intensity == 0:
                    continue
                
                current_time_ms += period
                time_s = current_time_ms / 1000.0
                
                times.append(time_s)
                intensities.append(intensity)
                bpms.append(bpm)
                temps.append(temp)
                
    except FileNotFoundError:
        print(f"File {filename} not found.")
        return

    if not times:
        print("No data found to plot.")
        return
        
    # Plotting
    fig, ax1 = plt.subplots(figsize=(12, 7))
    plt.subplots_adjust(right=0.75) # Make room for the third axis
    
    # Axis 1 (Left): Step Intensity
    color1 = 'tab:blue'
    ax1.set_xlabel(f'Time (s) [Total: {times[-1]:.1f}s]')
    ax1.set_ylabel('Intensity', color=color1)
    
    # Plot Intensity as scatter points
    l1 = ax1.plot(times, intensities, 'o', color=color1, label='Step Intensity', markersize=5, alpha=0.7)
    
    ax1.tick_params(axis='y', labelcolor=color1)
    ax1.grid(True, linestyle='--', alpha=0.5)
    
    # Axis 2 (Right): Avg BPM
    color2 = 'tab:red'
    ax2 = ax1.twinx()
    ax2.set_ylabel('Avg BPM', color=color2)
    
    # Plot BPM as a line
    l2 = ax2.plot(times, bpms, '-', color=color2, label='Avg BPM', linewidth=1.5, alpha=0.8)
    
    ax2.tick_params(axis='y', labelcolor=color2)
    
    # Axis 3 (Right + Offset): Temperature
    l3 = []
    if any(t > 0 for t in temps):
        color3 = 'tab:green'
        ax3 = ax1.twinx()
        # Offset the right spine of ax3
        ax3.spines["right"].set_position(("axes", 1.15))
        ax3.set_ylabel('Temperature (°C)', color=color3)
        l3 = ax3.plot(times, temps, 's-', color=color3, label='Temperature', linewidth=1.5, markersize=4)
        ax3.tick_params(axis='y', labelcolor=color3)
    
    plt.title('Step Intensity, Avg BPM, and Temperature')
    
    # Unified Legend
    lines = l1 + l2 + l3
    labels = [line.get_label() for line in lines]
    ax1.legend(lines, labels, loc='upper left')
    
    # fig.tight_layout() # tight_layout can conflict with explicit subplot adjustments

    
    # Save the plot
    output_filename = 'visualised_data_from_csv.png'
    plt.savefig(output_filename, dpi=300, bbox_inches='tight')
    print(f"Saved plot to '{output_filename}'")
    
    plt.show()

if __name__ == "__main__":
    generate_plot('step_data.csv')
