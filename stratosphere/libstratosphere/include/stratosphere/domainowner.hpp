#pragma once
#include <switch.h>
#include <type_traits>

#include "iserviceobject.hpp"

#define DOMAIN_ID_MAX 0x200

class IServiceObject;

class DomainOwner {
    private:
        IServiceObject *domain_objects[DOMAIN_ID_MAX];
    public:
        DomainOwner() {
            for (unsigned int i = 0; i < DOMAIN_ID_MAX; i++) {
                domain_objects[i] = NULL;
            }
        }
        
        virtual ~DomainOwner() {
            for (unsigned int i = 0; i < DOMAIN_ID_MAX; i++) {
                this->delete_object(i);
            }
        }
        
        IServiceObject *get_domain_object(unsigned int i) {
            if (i < DOMAIN_ID_MAX) {
                return domain_objects[i];
            }
            return NULL;
        }
        
        Result reserve_object(IServiceObject *object, unsigned int *out_i) {
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
        
        Result set_object(IServiceObject *object, unsigned int i) {
            if (domain_objects[i] == NULL) {
                domain_objects[i] = object;
                object->set_owner(this);
                return 0;
            }
            return 0x1900B;
        }
        
        unsigned int get_object_id(IServiceObject *object) {
            for (unsigned int i = 0; i < DOMAIN_ID_MAX; i++) {
                if (domain_objects[i] == object) {
                    return i;
                }
            }
            return DOMAIN_ID_MAX;
        }
        
        void delete_object(unsigned int i) {
            if (domain_objects[i]) {
                delete domain_objects[i];
                domain_objects[i] = NULL;
            }
        }
        
        void delete_object(IServiceObject *object) {
            for (unsigned int i = 0; i < DOMAIN_ID_MAX; i++) {
                if (domain_objects[i] == object) {
                    delete domain_objects[i];
                    domain_objects[i] = NULL;
                    break;
                }
            }
        }
};