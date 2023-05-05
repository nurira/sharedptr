#include <cstddef>
#include <mutex>

namespace cs540 {
    struct RefCountBase {
        unsigned *rC;
        virtual void ref() = 0;
        virtual bool deref() = 0;
    };
    template <typename T>
    struct RefCount : RefCountBase {
        T *r; std::mutex *l;
        RefCount() = delete;
        RefCount(T *ref) : r(ref), l(new std::mutex) { rC = new unsigned(1); }

        void ref() {
            l->lock();
            if (rC) { ++*rC; }
            l->unlock();
        }
        bool deref() {
            l->lock();
            // short circuit rC != NULL, decrement and clean up if == 0
            if (rC && !--*rC) {
                delete r; r = 0;
                delete rC; rC = 0;
                l->unlock();
                delete l; l = 0;
                return true;
            }
            l->unlock();
            return false;
        }
    };

    template <typename T> class SharedPtr {
        public:
            SharedPtr() : r(0), rC(0) {}
            template <typename U> explicit
            SharedPtr(U *p) : r(p), rC(new RefCount<U>(p)) {}

            SharedPtr(const SharedPtr &p) : r(p.r), rC(p.rC) {
                refer();
            }

            template <typename U>
            SharedPtr(const SharedPtr<U> &p) : r(p.r), rC(p.rC) {
                refer();
            }

            SharedPtr(T* p, RefCountBase *rp) : r(p), rC(rp) {
                refer();
            }

            SharedPtr(SharedPtr &&p) : r(p.r), rC(p.rC) {
                p.r = 0; p.rC = 0;
            }

            template <typename U>
            SharedPtr(SharedPtr<U> &&p) : r(p.r), rC(p.rC) {
                p.r = 0; p.rC = 0;
            }

            SharedPtr &operator=(const SharedPtr &p) {
                deref();
                r = p.r; rC = p.rC;
                refer();
                return *this;
            }
            template <typename U>
            SharedPtr &operator=(const SharedPtr<U> &p) {
                deref();
                r = p.r; rC = p.rC;
                refer();
                return *this;
            }

            SharedPtr &operator=(SharedPtr &&p) {
                deref();
                r = p.r; rC = p.rC; //steal
                p.r = 0; p.rC = 0;
                return *this;
            }

            template <typename U>
            SharedPtr &operator=(SharedPtr<U> &&p) {
                deref();
                r = p.r; rC = p.rC;
                p.r = 0; p.rC = 0;
                return *this;
            }

            ~SharedPtr() { deref(); }

            void reset() { deref(); }
            template <typename U>
            void reset(U *p) {
                deref();
                r = p;
                rC = new RefCount<U>(p);
            }

            unsigned int use_count() const { return (rC) ? *(rC->rC) : 0; }
            T *get() const { return r; }
            T &operator*() const { return *r; }
            T *operator->() const { return r; }
            explicit operator bool() const {
                return r == 0 ? false : true;
            }

            RefCountBase *rC;
            T *r;

        private:
            void refer() { if (rC) rC->ref(); }
            void deref() {
                if (rC && rC->deref()) delete rC;
                r = 0; rC = 0;
            }

    };

    template <typename T1, typename T2>
    bool operator==(const SharedPtr<T1> &p1, const SharedPtr<T2> &p2) {
        return p1.get() == p2.get();
    }

    template <typename T>
    bool operator==(const SharedPtr<T> &p, std::nullptr_t) {
        return !p.get();
    }

    template <typename T>
    bool operator==(std::nullptr_t, const SharedPtr<T> &p) {
        return !p.get();
    }

    template <typename T1, typename T2>
    bool operator!=(const SharedPtr<T1> &p1, const SharedPtr<T2> &p2) {
        return p1.get() != p2.get();
    }

    template <typename T>
    bool operator!=(const SharedPtr<T> &p, std::nullptr_t) {
        return !!p.get();
    }

    template <typename T>
    bool operator!=(std::nullptr_t, const SharedPtr<T> &p) {
        return !!p.get();
    }

    template <typename T, typename U>
    SharedPtr<T> static_pointer_cast(const SharedPtr<U> &sp) {
        return SharedPtr<T>(static_cast<T*>(sp.get()), sp.rC);
    }

    template <typename T, typename U>
    SharedPtr<T> dynamic_pointer_cast(const SharedPtr<U> &sp) {
        T* c = dynamic_cast<T*>(sp.get());
        return (c) ?
            SharedPtr<T>(dynamic_cast<T*>(sp.get()), sp.rC) : SharedPtr<T>{};
    }
}
