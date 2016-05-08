#include<iostream>
#include<vector>
#include<string>
#include<random>
#include<fstream>
#include<list>
#include<unordered_set>
#include<algorithm>
#include<numeric>

using std::cin;
using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::list;
using std::unordered_set;

// Represents an object in n-dim space.
class Object {
public:
    Object() {}
    vector<double> & Data() {
        return features_;
    }
    int Dim() const {
        return static_cast<int>(features_.size());
    }
    void Push(double value) {
        features_.push_back(value);
    }
    double operator[](int index) const {
        return features_.at(index);
    }
    void Print() const {
        for (auto feature : features_) {
            cout << feature << " ";
        }
        cout << endl;
    }
private:
    int dim_;
    vector<double> features_;
};

// Represents a cluster of objects.
struct Cluster {
    unordered_set<const Object *> objects;
    Object center;
    void Print() const {
        cout << "center of cluster:" << endl;
        center.Print();
        cout << "objects in cluster:" << endl;
        for (auto ob : objects) {
            ob->Print();
        }
        cout << endl;
    }
};

// Clusters objects with coordinates in [-limit, limit] by n_clusters clusters.
class Clusterer {
private:
    // Calculates distance between two objects.
    double Distance(const Object & ob1, const Object & ob2) {
        double distance = 0;
        for (int i = 0; i < dim; ++i) {
            distance += (ob1[i] - ob2[i]) * (ob1[i] - ob2[i]);
        }
        distance = sqrt(distance);
        return distance;
    }
    // Initializes clusters centers using random points.
    void GenerateRandomClusters(int n_clusters) {
        if (n_clusters > objects.size()) {
            throw "n_clusters > objects.size()";
        }
        std::default_random_engine engine(42);
        std::uniform_int_distribution<int> rnd(0, objects.size());
        vector<const Object *> unused;
        for (const Object & ob : objects) {
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
    // Calculates the center of mass.
    Object CalculateCenter(const std::unordered_set<const Object *> & points) {
        if (points.size() == 0) {
            throw "CalculateCenter: points.size() == 0.";
        }
        Object center;
        for (int coord = 0; coord < dim; ++coord) {
            double current_coord = 0;
            for (const Object * object : points) {
                current_coord += (*object)[coord];
            }
            current_coord /= points.size();
            center.Push(current_coord);
        }
        return center;
    }
    // Calculates objects for clusters with fixed centers.
    void ReClusterObjects() {
        for (Cluster & cluster : clusters) {
            cluster.objects.clear();
        }
        for (const Object & ob : objects) {
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

    // Calculates centers for clusters with fixed objects.
    bool ReCalculateCenters() {
        bool diff = false;
        for (Cluster & cluster : clusters) {
            Object old_center = cluster.center;
            Object new_center = CalculateCenter(cluster.objects);
            if (old_center.Data() != new_center.Data()) {
                diff = true;
            }
            cluster.center = new_center;
        }
        return diff;
    }
    vector<Cluster> clusters;
    const list<Object> & objects;
    int dim;
public:
    Clusterer(const list<Object> & objects_to_cluster, int n_clusters)
        : objects(objects_to_cluster) {
        dim = objects.back().Dim();
        GenerateRandomClusters(n_clusters);
        ReClusterObjects();
    }
    void Initialize() {
        bool diff = true;
        while (diff) {
            diff = ReCalculateCenters();
            ReClusterObjects();
            PrintClusters();
        }
    }
    void PrintClusters() const {
        cout << "   Strating clusters printing." << endl;
        int index = 0;
        for (const Cluster & cluster : clusters) {
            cout << "       Cluster index = " << index << endl;
            cluster.Print();
            ++index;
        }
        cout << "   End of clusters printing." << endl;
    }
    vector<Cluster> GetClusters() const {
        return clusters;
    }
};

list<Object> ReadObjects(std::istream & stream, int dim, int n_objects) {
    list<Object> objects;
    for (int i = 0; i < n_objects; ++i) {
        objects.push_back(Object());
        for (int j = 0; j < dim; ++j) {
            double num;
            stream >> num;
            objects.back().Push(num);
        }
    }
    return objects;
}

vector<Object> FindClustersCenters(const list<Object> & objects, int n_centers = 2) {
    Clusterer clusterer(objects, n_centers);
    clusterer.Initialize();
    
    vector<Cluster> clusters = clusterer.GetClusters();
    vector<Object> centers;
    for (const Cluster & cluster : clusters) {
        centers.push_back(cluster.center);
    }
    return centers;
}

int main() {
    /* \n */
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::ifstream stream("test.txt");

    cout << "dim, n_objects:" << endl;
    int dim, n_objects;
    stream >> dim >> n_objects;
    list<Object> objects = ReadObjects(stream, dim, n_objects);
    int n_centers;
    cout << "n_centers:" << endl;
    stream >> n_centers;

    vector<Object> centers = FindClustersCenters(objects, n_centers);

    for (const Object & center : centers) {
        center.Print();
    }

    system("pause");
    return 0;
}
