#include "dtiomod/dtiomod_client.h"

int main() {
  CHIMAERA_CLIENT_INIT();
  chi::dtiomod::Client client;
  client.Create(
		HSHM_MCTX,
		chi::DomainQuery::GetDirectHash(chi::SubDomainId::kGlobalContainers, 0),
		chi::DomainQuery::GetGlobalBcast(), "ipc_test");
  size_t data_size = hshm::Unit<size_t>::Megabytes(1);
  size_t data_offset = 0;
  hipc::FullPtr<char> orig_data =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, data_size);
  hipc::FullPtr<char> filename =
    CHI_CLIENT->AllocateBuffer(HSHM_MCTX, 15);
  char *filename_ptr = filename.ptr_;
  sprintf(filename_ptr, "dtio://test.txt");
  client.Write(HSHM_MCTX, orig_data.shm_,
	       data_size, data_offset, filename.shm_, 15, io_client_type::POSIX);
  return 0;
}
