import os
import glob
import matplotlib.pyplot as plt

def parse_summary(filepath):
    data = {}
    with open(filepath, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) == 2:
                data[parts[0]] = parts[1]
    return data

def main():
    results = []
    # Find all summary files in the results directory
    for file in glob.glob('results/*_summary.txt'):
        data = parse_summary(file)
        if not data:
            continue
            
        impl = data.get('implementation')
        particles = int(data.get('particles', 0))
        time = float(data.get('elapsed_seconds', 0))
        
        # Create a clean label based on implementation and degree of parallelism
        label = "Sequential"
        if impl == 'openmp':
            label = f"OpenMP ({data.get('threads', '1')} Threads)"
        elif impl == 'mpi':
            label = f"MPI ({data.get('processes', '1')} Processes)"
            
        results.append({'label': label, 'particles': particles, 'time': time})

    if not results:
        print("No summary files found in the 'results/' directory.")
        return

    # Group the read data by label
    grouped = {}
    for r in results:
        l = r['label']
        if l not in grouped:
            grouped[l] = {'x': [], 'y': []}
        grouped[l]['x'].append(r['particles'])
        grouped[l]['y'].append(r['time'])

    # Prepare the plot
    plt.figure(figsize=(10, 6))

    for label, data in grouped.items():
        # Sort data points by x (number of particles)
        sorted_pairs = sorted(zip(data['x'], data['y']))
        x = [p[0] for p in sorted_pairs]
        y = [p[1] for p in sorted_pairs]
        
        plt.plot(x, y, marker='o', label=label)

    plt.xlabel('Number of Particles (N)')
    plt.ylabel('Elapsed Time (seconds)')
    plt.title('N-Body Simulation Performance Comparison')
    plt.legend()
    plt.grid(True)
    
    # Save the graph
    output_path = os.path.join('results', 'performance_graph.png')
    plt.savefig(output_path)
    print(f"Graph successfully saved to: {output_path}")
    
    # Also show it on screen
    plt.show()

if __name__ == '__main__':
    main()
