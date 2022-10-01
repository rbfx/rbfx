set(SSL_SOURCE_FILES
    bio_ssl.c d1_lib.c d1_msg.c d1_srtp.c methods.c packet.c pqueue.c s3_cbc.c s3_enc.c s3_lib.c s3_msg.c ssl_asn1.c ssl_cert.c ssl_ciph.c ssl_conf.c ssl_err.c ssl_init.c ssl_lib.c ssl_mcnf.c ssl_quic.c ssl_rsa.c ssl_sess.c ssl_stat.c ssl_txt.c ssl_utst.c t1_enc.c t1_lib.c t1_trce.c tls13_enc.c tls_srp.c packet.c tls13_enc.c
    record/dtls1_bitmap.c record/rec_layer_d1.c record/rec_layer_s3.c record/ssl3_buffer.c record/ssl3_record.c record/ssl3_record_tls13.c
    statem/extensions.c statem/extensions_clnt.c statem/extensions_cust.c statem/extensions_srvr.c statem/statem.c statem/statem_clnt.c statem/statem_dtls.c statem/statem_lib.c statem/statem_quic.c statem/statem_srvr.c
)
add_library(ssl STATIC ${SSL_SOURCE_FILES})
target_link_libraries(ssl PUBLIC crypto)
if (WIN32)
    target_compile_definitions(ssl PRIVATE WIN32_LEAN_AND_MEAN)
endif ()
