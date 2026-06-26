$cases = @("32", "128", "512")
$configs = @(2, 4)

Write-Host "Cleaning up old result files..."
Remove-Item -Path "results\*.txt" -Force -ErrorAction SilentlyContinue

foreach ($c in $cases) {
    $inputFile = "data\case${c}.txt"
    
    Write-Host "----------------------------------------"
    Write-Host "Running Case $c"
    Write-Host "----------------------------------------"
    
    # 1. Sequential
    Write-Host " -> Sequential"
    .\sequential\nbody_seq.exe --input $inputFile --output "results\case${c}_seq_summary.txt"

    # 2. OpenMP (2 and 4 threads)
    foreach ($threadCount in $configs) {
        Write-Host " -> OpenMP ($threadCount threads)"
        .\openmp\nbody_omp.exe --input $inputFile --threads $threadCount --output "results\case${c}_omp${threadCount}_summary.txt"
    }

    # 3. MPI (2 and 4 processes)
    foreach ($processCount in $configs) {
        Write-Host " -> MPI ($processCount processes)"
        mpiexec -n $processCount .\mpi\nbody_mpi.exe --input $inputFile --output "results\case${c}_mpi${processCount}_summary.txt"
    }
}
Write-Host "========================================"
Write-Host "All experiments successfully completed!"
Write-Host "========================================"
