#pragma once
#include<iostream>
#include<vector>
#include<string>
#include<random>
#include<fstream>
#include<list>
#include<unordered_set>
#include<algorithm>
#include<numeric>
#include<thread>
#include<sstream>

using std::cout;
using std::endl;

class Cluster;
// Represents an point in n-dim space.
class Object {
    friend Cluster;
public:
	Object() {}
    Object(int dim) {
        for (int i = 0; i < dim; ++i) {
            features_.push_back(0);
        }
    }
	// Returns the inner vector.
	std::vector<double> & Data() {
		return features_;
	}
	const std::vector<double> & Data() const {
		return features_;
	}
	int Dim() const {
		return static_cast<int>(features_.size());
	}
	// Adds new coordinate.
	void Push(double value) {
		features_.push_back(value);
	}
	// Returns coordinate number index.
	double operator[](int index) const {
		return features_.at(index);
	}
    void SetNull() {
        for (double & coordinate : features_) {
            coordinate = 0;
        }
    }

    Object operator+=(const Object & other) {
        if (Dim() != other.Dim()) {
            throw "Objects have different dimensions.";
        }
        for (int iter = 0; iter < Dim(); ++iter) {
            features_[iter] += other.features_[iter];
        }
        return *this;
    }

    bool operator==(const Object & other) const {
        if (Dim() != other.Dim()) {
            throw "Objects have different dimensions.";
        }
        for (int i = 0; i < Dim(); ++i) {
            if (features_[i] != other.features_[i]) {
                return false;
            }
        }
        return true;
    }

    Object operator/=(int number) {
        for (int iter = 0; iter < Dim(); ++iter) {
            features_[iter] /= number;
        }
        return *this;
    }

	void Print(std::ostream & stream = std::cout) const {
		for (auto feature : features_) {
			stream << feature << " ";
		}
		stream << std::endl;
	}

protected:
	std::vector<double> features_;
};

class Cluster : public Object {
public:
    Cluster & operator=(const Object & other) {
        this->features_ = other.features_;
        return *this;
    }
    Cluster() : n_points_near(0) {}
    Cluster(int dim) : n_points_near(0) {
        for (int i = 0; i < dim; ++i) {
            features_.push_back(0);
        }
    }
    void SetNull() {
        for (double & coordinate : features_) {
            coordinate = 0;
        }
        n_points_near = 0;
    }
    int n_points_near;
};

// Calculates distance between two objects.
double Distance(const Object & ob1, const Object & ob2) {
    double distance = 0;
    int dim = ob1.Dim();
    for (int i = 0; i < dim; ++i) {
        distance += (ob1[i] - ob2[i]) * (ob1[i] - ob2[i]);
    }
    distance = sqrt(distance);
    return distance;
}

// Slave Client
void CalculateNearestClusters(
    const std::vector<Object> objects, 
    const std::vector<Cluster> & old_cluters,
    std::vector<Cluster> & new_clusters) {
    new_clusters.resize(old_cluters.size(), Cluster(old_cluters[0].Dim()));
    for (const auto & ob : objects) {
        int min_index = 0;
        double min_distance = 1000000000;
        for (int index = 0; index < old_cluters.size(); ++index) {
            double dist = Distance(ob, old_cluters[index]);
            if (dist < min_distance) {
                min_distance = dist;
                min_index = index;
            }
        }
        new_clusters[min_index] += ob;
        ++new_clusters[min_index].n_points_near;
    }
}

// Reads pool, divides it in parts, starts slave clients.
class Client {
public:
	Client(int dim, int n_clusters, int n_jobs, std::string input_file_name)
		: dim_(dim)
		, n_clusters_(n_clusters)
        , n_jobs_(n_jobs)
		, input_file_name_(input_file_name) {}

	// Clusters.
	void ProcessStreamPool() {
		ReadPool();
		cout << "Start of clustering objects" << endl; // for debugging and process supervision.
		GenerateRandomClusters();
		int iteration = 0; // for debugging and process supervision.
        for (const auto & cluster : clusters_) {
            cluster.Print();
        }
		while (Recluster()) {
			if (iteration % 10 == 0) {
				cout << "Clustering iteration number " << iteration << "." << endl; // for debugging and process supervision.
			}
			++iteration;
		}
		cout << "End of clustering objects" << endl; // for debugging and process supervision.
	}

	// Prints contained clusters centers and points.
	void PrintClusters() const {
		std::cout << "   Strating clusters printing." << std::endl;
		int index = 0;
		for (const Object & cluster : clusters_) {
			std::cout << "       Cluster index = " << index << std::endl;
			cluster.Print();
			++index;
		}
		std::cout << "   End of clusters printing." << std::endl;
	}

	// Returns contained clusters.
	std::vector<Cluster> GetClusters() const {
		return clusters_;
	}

private:
	// Objects for clustering.
    std::vector<std::vector<Object>> object_parts_;

	int dim_, n_clusters_, n_jobs_;
    int n_objects_;
	std::string input_file_name_;
	std::vector<Cluster> clusters_;

    // Reads pool and divides it in parts for multithreading.
	void ReadPool() {
		std::ifstream stream(input_file_name_);
		std::string current_line;
        int part_iter = 0;
        object_parts_.resize(n_jobs_,  std::vector<Object>());
		while (std::getline(stream, current_line)) {
            object_parts_[part_iter].push_back(Object());
			std::stringstream current_stream(current_line);
			int index;
			current_stream >> index;
			double coordinate;
			while (current_stream >> coordinate) {
                object_parts_[part_iter].back().Push(coordinate);
			}
            ++part_iter;
            if (part_iter == n_jobs_) {
                part_iter = 0;
            }
		}
        n_objects_ = 0;
        for (const auto part : object_parts_) {
            n_objects_ += part.size();
        }
		/* begin of debugging cout */
		std::cout << "End of reading pool." << std::endl;
		std::cout << "The number of objects for clustering is " << n_objects_ << "." << std::endl;
		std::cout << endl;
		/* end of debugging cout */
	}

	// Initializes clusters centers using random points.
	void GenerateRandomClusters() {
		std::default_random_engine engine(42);
		std::uniform_int_distribution<int> rnd(0, n_objects_);
		std::vector<const Object *> unused;
        for (const auto & part : object_parts_) {
            for (const auto & ob : part) {
                unused.push_back(&ob);
            }
        }

		for (int i = 0; i < n_clusters_; ++i) {
            Cluster cluster;
			int unused_index = rnd(engine) % unused.size();
			cluster = *(unused[unused_index]);
			unused.erase(unused.begin() + unused_index);
			clusters_.push_back(cluster);
		}
	}

    bool Recluster() {
        std::vector<Cluster> old_clusters = clusters_;
        std::vector<std::vector<Cluster>> new_clusters(n_jobs_);
        std::vector<std::thread> threads;
        for (int i = 0; i < n_jobs_; ++i) {
            threads.push_back(std::thread(
                CalculateNearestClusters, 
                std::cref(object_parts_[i]), 
                std::cref(old_clusters), 
                std::ref(new_clusters[i])));
        }
        for (auto & cluster : clusters_) {
            cluster.SetNull();
        }
        for (std::thread & thread : threads) {
            thread.join();
        }
        for (int i_job = 0; i_job < n_jobs_; ++i_job) {
            for (int i_cluster = 0; i_cluster < clusters_.size(); ++i_cluster) {
                clusters_[i_cluster] += new_clusters[i_job][i_cluster];
                clusters_[i_cluster].n_points_near += new_clusters[i_job][i_cluster].n_points_near;
            }
        }
        for (auto & cluster : clusters_) {
            cluster /= cluster.n_points_near;
        }
        if (old_clusters != clusters_) {
            return true;
        } else {
            return false;
        }
    }
};
