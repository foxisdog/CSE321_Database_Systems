#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>

struct Student {
    int id;
    std::string name;
    std::string gender;
    float gpa;
    float height;
    float weight;
};

struct Storage {
    std::vector<Student> records;

    Storage(const std::string& filepath){
        std::ifstream file(filepath);
        std::string line;
        std::getline(file, line); // 헤더 날리기

        while (std::getline(file, line)){
            std::stringstream ss(line);
            std::string token;
            Student s;

            std::getline(ss, token, ',');
            s.id = std::stoi(token);
            std::getline(ss, s.name, ',');
            std::getline(ss, s.gender, ',');

            std::getline(ss, token, ',');
            s.gpa = std::stof(token);

            std::getline(ss, token, ',');
            s.height = std::stof(token);

            std::getline(ss, token, ',');
            s.weight = std::stof(token);

            records.push_back(s);
        }
    }

    const Student& get(int rid) const { return records[rid]; }
    int size() const { return records.size(); }
};
