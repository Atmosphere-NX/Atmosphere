#pragma once
#include <switch.h>
#include <algorithm>
#include <memory>
#include <type_traits>

#include "iserviceobject.hpp"

#define DOMAIN_ID_MAX 0x1000

class IServiceObject;

class DomainOwner {
    private:
        std::array<std::shared_ptr<IServiceObject>, DOMAIN_ID_MAX> domain_objects;
    public:
        /* Shared ptrs should auto delete here. */
        virtual ~DomainOwner() = default;

        std::shared_ptr<IServiceObject> get_domain_object(unsigned int i) {
            if (i < DOMAIN_ID_MAX) {
                return domain_objects[i];
            }
            return nullptr;
        }
        
        Result reserve_object(std::shared_ptr<IServiceObject> object, unsigned int *out_i) {
            auto object_it = std::find(domain_objects.begin() + 4, domain_objects.end(), nullptr);
            if (object_it == domain_objects.end()) {
                return 0x1900B;
            }

            *out_i = std::distance(domain_objects.begin(), object_it);
            *object_it = object;
            object->set_owner(this);
            return 0;
        }
        
        Result set_object(std::shared_ptr<IServiceObject> object, unsigned int i) {
            if (domain_objects[i] == nullptr) {
                domain_objects[i] = object;
                object->set_owner(this);
                return 0;
            }
            return 0x1900B;
        }
        
        unsigned int get_object_id(std::shared_ptr<IServiceObject> object) {
            auto object_it = std::find(domain_objects.begin(), domain_objects.end(), object);
            return std::distance(domain_objects.begin(), object_it);
        }
        
        void delete_object(unsigned int i) {
            domain_objects[i].reset();
        }
        
        void delete_object(std::shared_ptr<IServiceObject> object) {
            auto object_it = std::find(domain_objects.begin(), domain_objects.end(), object);
            if (object_it != domain_objects.end()) {
                object_it->reset();
            }
        }
};
