#SGX_MODE ?= SIM
SGX_MODE ?= HW
SGX_PRERELEASE=1
SGX_ARCH ?= x64
EPCPP_CACHE_SIZE ?= "-DCACHE_SIZE=67108864"
export EPCPP_CACHE_SIZE
RANDOM_ACCESS ?= "-DAPTR_RANDOM_ACCESS"
export RANDOM_ACCESS

# NOSANITY="-DNOSANITY"
# export NOSANITY
# to disable sanity checks , uncomment
all: 
	$(MAKE) -C ../../eleos_core/trustedlib_lib_services  all 
	$(MAKE) -f sgx_u.mk all
	$(MAKE) -f sgx_t.mk all

clean:
	$(MAKE) -C ../../eleos_core/trustedlib_lib_services clean
	$(MAKE) -f sgx_u.mk clean
	$(MAKE) -f sgx_t.mk clean

