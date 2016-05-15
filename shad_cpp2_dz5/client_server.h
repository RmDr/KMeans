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


// Represents an object in n-dim space.
class Object {
public:
	Object() {}
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
	void Print() const {
		for (auto feature : features_) {
			std::cout << feature << " ";
		}
		std::cout << std::endl;
	}
private:
	int dim_;
	std::vector<double> features_;
};

// Represents a cluster of objects.
struct Cluster {
	// Pointers on clustered objects.
	std::unordered_set<const Object *> objects;
	// The center of mass (usually) of clustered objects.
	Object center;
	void Print() const {
		std::cout << "center of cluster:" << std::endl;
		center.Print();
		std::cout << "objects in cluster:" << std::endl;
		for (auto ob : objects) {
			ob->Print();
		}
		std::cout << std::endl;
	}
};

// Slave Client
class CenterOfMassCalculator {
public:
	void operator()(const std::unordered_set<const Object *> & points, Object & result) {
		std::cout << points.size() << std::endl;
		if (points.size() == 0) {
			cout << "CalculateCenter: points.size() == 0." << endl;
			throw "CalculateCenter: points.size() == 0.";
		}
		int dim = (*points.begin())->Dim();
		for (int coord = 0; coord < dim; ++coord) {
			double current_coord = 0;
			for (const Object * object : points) {
				current_coord += (*object)[coord];
			}
			current_coord /= points.size();
			result.Push(current_coord);
		}
	}
};

// Reads pool, divides it in parts, starts slave clients.
class Client {
public:
	Client(int n_jobs = 2, int n_clusters = 2)
		: n_jobs_(n_jobs)
		, n_clusters_(n_clusters) {}

	void ProcessStreamPool() {
		ReadPool();
		dim_ = objects_.back().Dim();
		GenerateRandomClusters(n_clusters_);
		ReClusterObjects();

		if (n_jobs_ > n_clusters_) {
			n_jobs_ = n_clusters_;
		}
		part_size_ = objects_.size() % n_jobs_;

		bool diff = true;
		while (diff) {
			diff = ReCalculateCenters();
			ReClusterObjects();
			PrintClusters();
		}
	}

	void PrintClusters() const {
		std::cout << "   Strating clusters printing." << std::endl;
		int index = 0;
		for (const Cluster & cluster : clusters) {
			std::cout << "       Cluster index = " << index << std::endl;
			cluster.Print();
			++index;
		}
		std::cout << "   End of clusters printing." << std::endl;
	}

	std::vector<Cluster> GetClusters() const {
		return clusters;
	}

private:
	std::list<Object> objects_;
	int n_jobs_, n_clusters_, dim_, part_size_;
	std::vector<Cluster> clusters;

	void ReadPool() {
		std::ifstream stream("test.txt");
		std::string current_line;
		while (std::getline(stream, current_line)) {
			objects_.push_back(Object());
			std::stringstream current_stream(current_line);
			int index;
			current_stream >> index;
			double coordinate;
			while (current_stream >> coordinate) {
				objects_.back().Push(coordinate);
			}
		}
		std::cout << objects_.size() << std::endl;
		objects_.back().Print();
	}

	// Calculates distance between two objects.
	double Distance(const Object & ob1, const Object & ob2) {
		double distance = 0;
		for (int i = 0; i < dim_; ++i) {
			distance += (ob1[i] - ob2[i]) * (ob1[i] - ob2[i]);
		}
		distance = sqrt(distance);
		return distance;
	}

	// Initializes clusters centers using random points.
	void GenerateRandomClusters(int n_clusters) {
		if (n_clusters > objects_.size()) {
			throw "n_clusters > objects.size()";
		}
		std::default_random_engine engine(42);
		std::uniform_int_distribution<int> rnd(0, (int)(objects_.size()));
		std::vector<const Object *> unused;
		for (const Object & ob : objects_) {
			unused.push_back(&ob);
		}
		for (int i = 0; i < n_clusters; ++i) {
			Cluster cluster;

			int unused_index = rnd(engine) % unused.size();
			cluster.center = *(unused[unused_index]);
			unused.erase(unused.begin() + unused_index);

			clusters.push_back(cluster);
		}
	}

	// Calculates centers for clusters with fixed objects.
	bool ReCalculateCenters() {
		std::vector<std::thread> threads;
		std::vector<Object> old_centers;
		std::vector<Object> new_centers;
		for (const Cluster & cluster : clusters) {
			old_centers.push_back(cluster.center);
			new_centers.push_back(Object());
		}
		for (int i = 0; i < clusters.size(); ++i) {
			CenterOfMassCalculator calculator;
			threads.push_back(std::thread(CenterOfMassCalculator(), std::ref(clusters[i].objects), std::ref(new_centers[i])));
		}

		for (auto & thread : threads) {
			thread.join();
		}
		bool diff = false;
		for (int i = 0; i < old_centers.size(); ++i) {
			if (old_centers[i].Data() != new_centers[i].Data()) {
				diff = true;
			}
			clusters[i].center = new_centers[i];
		}
		return diff;
	}

	// Calculates objects for clusters with fixed centers.
	void ReClusterObjects() {
		for (Cluster & cluster : clusters) {
			cluster.objects.clear();
		}
		for (const Object & ob : objects_) {
			Cluster * nearest = &clusters.back();
			double min_distance = Distance(nearest->center, ob);
			for (Cluster & cluster : clusters) {
				double distance = Distance(cluster.center, ob);
				if (distance < min_distance) {
					min_distance = distance;
					nearest = &cluster;
				}
			}
			nearest->objects.insert(&ob);
		}
	}


};
