#include <iostream>
#include <random>
#include <thread>
#include <future>
#include <chrono>

const int numThreads = std::thread::hardware_concurrency();

void printMatrix(const std::vector<std::vector<int>>& Matrix, int n){
  for (int i = 0; i < n; i++){
    std::cout << "| ";
    for (int j = 0; j < n; j++) std::cout << Matrix[i][j] << " | ";
    std::cout << "\n";
  }
}

// This functions generates a squared matrix of size n
// We consider a sparse matrix to be one in which the
// number of non-zero numbers is approximately the 
// same as the number of rows or columns.
// We will make this probabilistically by using a 
// probability of 1/n
void SparseMatrixGenerator(std::vector<std::vector<int>>& Matrix, int n){ 
  auto generateRows = [&](int startRow, int endRow) {
    float prob = 1.0 / n; 
    std::random_device rd;
    std::mt19937 gen(rd());  
    std::uniform_real_distribution<> dis(0.0, 1.0); 
    std::uniform_int_distribution<> randnum(1, 100); 

    for (int i = startRow; i < endRow; ++i) {
      for (int j = 0; j < n; ++j) if (dis(gen) < prob) Matrix[i][j] = randnum(gen); 
    }
  };

  std::vector<std::future<void>> futures;
  int rowsPerThread = n / numThreads;
  int remainder = n % numThreads;

  for (int t = 0; t < numThreads; ++t) {
  int startRow = t * rowsPerThread + std::min(t, remainder);
  int endRow = startRow + rowsPerThread;
  if (t < remainder) endRow += 1; 

  futures.push_back(std::async(std::launch::async, generateRows, startRow, endRow));
  }

  for (auto& fut : futures) fut.get();
}


void singleClassicalMultiplication(std::vector<std::vector<int>> M1,
                                   std::vector<std::vector<int>> M2, 
                                   std::vector<std::vector<int>>& M3, int n) {
  for (int i = 0; i < n; i++){
    for (int j = 0; j < n; j++){
      for (int k = 0; k < n; k++)  M3[i][j] += M1[i][k] * M2[k][j];
    }
  }
}

void threadedClassicalMultiplication(std::vector<std::vector<int>>& M1,
                                     std::vector<std::vector<int>>& M2, 
                                     std::vector<std::vector<int>>& M3, 
                                     int n) {
    auto multiplyRows = [&](int startRow, int endRow) {
        for (int i = startRow; i < endRow; ++i) {
            for (int j = 0; j < n; ++j) {
                for (int k = 0; k < n; ++k) M3[i][j] += M1[i][k] * M2[k][j]; 
          }
        }
    };

    int rowsPerThread = n / numThreads;
    int remainder = n % numThreads;

    std::vector<std::future<void>> futures;

    for (int t = 0; t < numThreads; ++t) {
        int startRow = t * rowsPerThread + std::min(t, remainder);
        int endRow = startRow + rowsPerThread;
        if (t < remainder) endRow += 1;
        futures.push_back(std::async(std::launch::async, multiplyRows, startRow, endRow));
    }

    for (auto& fut : futures) fut.get();
}

void matrixToCOO(std::vector<std::vector<int>>& M, int n,
                 std::vector<int>& rows,
                 std::vector<int>& cols,
                 std::vector<int>& vals) {
    int rowsPerThread = n / numThreads;
    int remainder = n % numThreads;

    std::vector<std::vector<int>> threadRows(numThreads);
    std::vector<std::vector<int>> threadCols(numThreads);
    std::vector<std::vector<int>> threadVals(numThreads);

    auto processRows = [&](int threadID, int startRow, int endRow) {
        for (int i = startRow; i < endRow; ++i) {
            for (int j = 0; j < n; ++j) {
                if (M[i][j] != 0) {
                    threadRows[threadID].push_back(i);
                    threadCols[threadID].push_back(j);
                    threadVals[threadID].push_back(M[i][j]);
                }
            }
        }
    };

    std::vector<std::future<void>> futures;
    for (int t = 0; t < numThreads; ++t) {
        int startRow = t * rowsPerThread + std::min(t, remainder);
        int endRow = startRow + rowsPerThread;
        if (t < remainder) endRow += 1;

        futures.push_back(std::async(std::launch::async, processRows, t, startRow, endRow));
    }

    for (auto& fut : futures) fut.get();

    for (int t = 0; t < numThreads; ++t) {
        rows.insert(rows.end(), threadRows[t].begin(), threadRows[t].end());
        cols.insert(cols.end(), threadCols[t].begin(), threadCols[t].end());
        vals.insert(vals.end(), threadVals[t].begin(), threadVals[t].end());
    }
}

void multiplyCOO(const std::vector<int>& rowsA, const std::vector<int>& colsA, const std::vector<int>& valsA,
                 const std::vector<int>& rowsB, const std::vector<int>& colsB, const std::vector<int>& valsB,
                 std::vector<int>& rowsC, std::vector<int>& colsC, std::vector<int>& valsC) {
    std::unordered_map<int, std::unordered_map<int, int>> result;
    for (size_t i = 0; i < rowsA.size(); ++i) {
        int rowA = rowsA[i];
        int colA = colsA[i];
        int valA = valsA[i];

        for (size_t j = 0; j < rowsB.size(); ++j) {
            if (rowsB[j] == colA) {
                int colB = colsB[j];
                int valB = valsB[j];
                result[rowA][colB] += valA * valB;
            }
        }
    }
    for (const auto& [row, colMap] : result) {
        for (const auto& [col, value] : colMap) {
            if (value != 0) {
                rowsC.push_back(row);
                colsC.push_back(col);
                valsC.push_back(value);
            }
        }
    }
}

void multiMultiplyCOO(const std::vector<int>& rowsA, const std::vector<int>& colsA, const std::vector<int>& valsA,
                 const std::vector<int>& rowsB, const std::vector<int>& colsB, const std::vector<int>& valsB,
                 std::vector<int>& rowsC, std::vector<int>& colsC, std::vector<int>& valsC) {
size_t elementsPerThread = rowsA.size() / numThreads;
    size_t remainder = rowsA.size() % numThreads;

    std::vector<std::future<std::unordered_map<int, std::unordered_map<int, int>>>> futures;

    auto threadTask = [&](size_t start, size_t end) {
        std::unordered_map<int, std::unordered_map<int, int>> localResult;
        for (size_t i = start; i < end; ++i) {
            int rowA = rowsA[i];
            int colA = colsA[i];
            int valA = valsA[i];

            for (size_t j = 0; j < rowsB.size(); ++j) {
                if (rowsB[j] == colA) {
                    int colB = colsB[j];
                    int valB = valsB[j];
                    localResult[rowA][colB] += valA * valB;
                }
            }
        }
        return localResult;
    };

    size_t start = 0;
    for (int t = 0; t < numThreads; ++t) {
        size_t end = start + elementsPerThread + (t < remainder ? 1 : 0);
        futures.push_back(std::async(std::launch::async, threadTask, start, end));
        start = end;
    }

    std::unordered_map<int, std::unordered_map<int, int>> result;
    for (auto& future : futures) {
        auto localResult = future.get();
        for (const auto& [row, colMap] : localResult) {
            for (const auto& [col, value] : colMap) {
                result[row][col] += value;
            }
        }
    }

    for (const auto& [row, colMap] : result) {
        for (const auto& [col, value] : colMap) {
            if (value != 0) {
                rowsC.push_back(row);
                colsC.push_back(col);
                valsC.push_back(value);
            }
        }
    }
}

int main(){
  int matrixSize;
  std::cout << "Choose your matrices size\n";
  std::cin >> matrixSize;

  std::vector<int> rows1; std::vector<int> rows2; std::vector<int> rows3;
  std::vector<int> cols1; std::vector<int> cols2; std::vector<int> cols3;
  std::vector<int> vals1; std::vector<int> vals2; std::vector<int> vals3;
  std::vector<std::vector<int>> M1(matrixSize, std::vector<int>(matrixSize, 0));
  std::vector<std::vector<int>> M2(matrixSize, std::vector<int>(matrixSize, 0));

  auto start = std::chrono::high_resolution_clock::now();
  SparseMatrixGenerator(M1, matrixSize);
  SparseMatrixGenerator(M2, matrixSize);
  auto end = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "Generation time: " << duration.count() << std::endl;

  // Matrices to store results
  std::vector<std::vector<int>> M3(matrixSize, std::vector<int>(matrixSize, 0)); 
  std::vector<std::vector<int>> M4(matrixSize, std::vector<int>(matrixSize, 0));

  start = std::chrono::high_resolution_clock::now();
  matrixToCOO(M1, matrixSize, rows1, cols1, vals1);
  matrixToCOO(M2, matrixSize, rows2, cols2, vals2);
  end = std::chrono::high_resolution_clock::now();
  duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  start = std::chrono::high_resolution_clock::now();
  singleClassicalMultiplication(M1,M2, M3, matrixSize);
  end = std::chrono::high_resolution_clock::now();

  duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "Singlethreaded classical multiplication time: " << duration.count() << std::endl;

  start = std::chrono::high_resolution_clock::now();
  threadedClassicalMultiplication(M1,M2, M4, matrixSize);
  end = std::chrono::high_resolution_clock::now();

  duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "Multithreaded classical multiplication time: " << duration.count() << std::endl;

  start = std::chrono::high_resolution_clock::now();
  multiplyCOO(rows1, cols1, vals1, rows2, cols2, vals2, rows3, cols3, vals3); 
  end = std::chrono::high_resolution_clock::now();

  duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "Coo multiplication time: " << duration.count() << std::endl;


  start = std::chrono::high_resolution_clock::now();
  multiMultiplyCOO(rows1, cols1, vals1, rows2, cols2, vals2, rows3, cols3, vals3); 
  end = std::chrono::high_resolution_clock::now();

  duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "Multithreaded Coo multiplication time: " << duration.count() << std::endl;
/*  printMatrix(M1, matrixSize);*/
/*  printMatrix(M2, matrixSize);*/
  /*printMatrix(M3, matrixSize);*/
  return 0;
}
