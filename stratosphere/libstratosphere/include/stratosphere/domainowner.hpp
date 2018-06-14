#pragma once
#include <switch.h>
#include <memory>
#include <type_traits>

#include "iserviceobject.hpp"

#define DOMAIN_ID_MAX 0x1000

class IServiceObject;

class DomainOwner {
    private:
        std::shared_ptr<IServiceObject> domain_objects[DOMAIN_ID_MAX];
    public:
        DomainOwner() {
            for (unsigned int i = 0; i < DOMAIN_ID_MAX; i++) {
                domain_objects[i].reset();
            }
        }
        
        virtual ~DomainOwner() {
            /* Shared ptrs should auto delete here. */
        }
        
        std::shared_ptr<IServiceObject> get_domain_object(unsigned int i) {
            if (i < DOMAIN_ID_MAX) {
                return domain_objects[i];
            }
            return nullptr;
        }
        
        Result reserve_object(std::shared_ptr<IServiceObject> object, unsigned int *out_i) {
            for (unsigned int i = 4; i < DOMAIN_ID_MAX; i++) {
                if (domain_objects[i] == NULL) {
                    domain_objects[i] = object;
                    object->set_owner(this);
                    *out_i = i;
                    return 0;
                }
            }
            return 0x1900B;
        }
        
        Result set_object(std::shared_ptr<IServiceObject> object, unsigned int i) {
            if (domain_objects[i] == NULL) {
                domain_objects[i] = object;
                object->set_owner(this);
                return 0;
            }
            return 0x1900B;
        }
        
        unsigned int get_object_id(std::shared_ptr<IServiceObject> object) {
            for (unsigned int i = 0; i < DOMAIN_ID_MAX; i++) {
                if (domain_objects[i] == object) {
                    return i;
                }
            }
            return DOMAIN_ID_MAX;
        }
        
        void delete_object(unsigned int i) {
            if (domain_objects[i]) {
                domain_objects[i].reset();
            }
        }
        
        void delete_object(std::shared_ptr<IServiceObject> object) {
            for (unsigned int i = 0; i < DOMAIN_ID_MAX; i++) {
                if (domain_objects[i] == object) {
                    domain_objects[i].reset();
                    break;
                }
            }
        }
};