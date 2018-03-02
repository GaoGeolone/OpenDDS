/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "gtest/gtest.h"
#include "dds/DCPS/security/AuthenticationBuiltInImpl.h"
#include "dds/DCPS/GuidUtils.h"
#include <cstring>

using OpenDDS::DCPS::GUID_t;
using OpenDDS::Security::AuthenticationBuiltInImpl;
using DDS::DomainParticipantQos;
using DDS::Property_t;
using DDS::PropertySeq;
using DDS::BinaryProperty_t;
using DDS::BinaryPropertySeq;
using DDS::Security::SecurityException;
using DDS::Security::IdentityHandle;
using DDS::Security::IdentityStatusToken;
using DDS::Security::IdentityToken;
using DDS::Security::PermissionsCredentialToken;
using DDS::Security::PermissionsToken;
using DDS::Security::AuthRequestMessageToken;
using DDS::Security::ValidationResult_t;
using DDS::Security::DomainId_t;

struct AuthenticationTest : public ::testing::Test
{
  static IdentityHandle ValidateLocalParticipant(AuthenticationBuiltInImpl& test_class)
  {
    // Just excercise the interface and return the handle/status
    // No GUID or QoS checking is needed at this time
    // THis also never fails in the stub, so don't bother checking
    IdentityHandle local_handle = 0;
    GUID_t adjusted_participant_guid;
    GUID_t initial_guid;
    DomainParticipantQos qos;
    SecurityException ex;

    test_class.validate_local_identity(
      local_handle,
      adjusted_participant_guid,
      1,
      qos,
      initial_guid,
      ex);

      return local_handle;
  }

  static void CallFunctionsWithInvalidLocalHandle(
    AuthenticationBuiltInImpl& test_class,
    IdentityHandle handle)
  {
    SecurityException ex;

    // Can't get or set tokens without a validated handle
    IdentityToken token;
    IdentityStatusToken status_token;
    EXPECT_FALSE(test_class.get_identity_token(token, handle, ex));
    EXPECT_FALSE(test_class.get_identity_token(status_token, handle, ex));

    PermissionsCredentialToken cred_token;
    PermissionsToken perm_token;
    EXPECT_FALSE(test_class.set_permissions_credential_and_token(handle, cred_token, perm_token, ex));

    // Can't validate a remote paticipant without validating local first
    IdentityHandle remote_handle_out = 0;
    IdentityToken remote_token;
    AuthRequestMessageToken local_auth_request_token;
    AuthRequestMessageToken remote_auth_request_token;
    GUID_t remote_guid;
    ValidationResult_t validate_result =
      test_class.validate_remote_identity(
        remote_handle_out,
        local_auth_request_token,
        remote_auth_request_token,
        handle,
        remote_token,
        remote_guid,
        ex);
    EXPECT_EQ(DDS::Security::VALIDATION_FAILED, validate_result);
  }

  AuthenticationTest() : guid(OpenDDS::DCPS::GUID_UNKNOWN) {
      Property_t idca, pkey, pass, idcert;

      idca.name = "dds.sec.auth.identity_ca";
      idca.value = "file:certs/opendds_identity_ca_cert.pem";
      idca.propagate = false;

      pkey.name = "dds.sec.auth.private_key";
      pkey.value = "file:certs/mock_participant_1/opendds_participant_private_key.pem";
      pkey.propagate = false;

      pass.name = "dds.sec.auth.password";
      pass.value = "";
      pass.propagate = false;

      idcert.name = "dds.sec.auth.identity_certificate";
      idcert.value = "file:certs/mock_participant_1/opendds_participant_cert.pem";
      idcert.propagate = false;

      add_property(idca);
      add_property(pkey);
      add_property(pass);
      add_property(idcert);
  }

  ~AuthenticationTest(){
  }

  void add_property(Property_t p) {
    PropertySeq& seq = domain_participant_qos.property.value;
    size_t len = seq.length();
    seq.length(len + 1);
    seq[len] = p;
  }

  void add_binary_property(BinaryProperty_t p) {
    BinaryPropertySeq& seq = domain_participant_qos.property.binary_value;
    size_t len = seq.length();
    seq.length(len + 1);
    seq[len] = p;
  }

  static std::string value_of(const std::string& key, const PropertySeq& s)
  {
    size_t n = s.length();
    for (size_t i = 0; i < n; ++i) {
        if (strcmp(s[i].name, key.c_str()) == 0)
          return static_cast<const char*>(s[i].value);
    }
    return NULL;
  }

  DomainParticipantQos domain_participant_qos;
  GUID_t guid;
  DomainId_t domain_id;
  SecurityException ex;
};

TEST_F(AuthenticationTest, ValidateLocalIdentity_Success)
{
  AuthenticationBuiltInImpl auth;
  IdentityHandle h;
  GUID_t adjusted;
  ValidationResult_t r = auth.validate_local_identity(h, adjusted, domain_id, domain_participant_qos, guid, ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_OK);
}

TEST_F(AuthenticationTest, GetIdentityToken_Success)
{
  /* From this cmd:  openssl x509 -noout -subject -in certs/opendds_participant_cert.pem */
  std::string cert_sn("C = US, ST = MO, O = Object Computing (Test Identity CA), CN = Object Computing (Test Identity CA), emailAddress = info@objectcomputing.com");

  /* Same thing but with certs/opendds_identity_ca_cert.pem */
  std::string ca_sn("C = US, ST = MO, L = Saint Louis, O = Object Computing (Test Identity CA), CN = Object Computing (Test Iden CA), emailAddress = info@objectcomputing.com");

  AuthenticationBuiltInImpl auth;
  IdentityHandle h;
  IdentityToken t;
  GUID_t adjusted;

  auth.validate_local_identity(h, adjusted, domain_id, domain_participant_qos, guid, ex);

  ASSERT_EQ(true, auth.get_identity_token(t, h, ex));

  ASSERT_EQ(cert_sn, value_of("dds.cert.sn", t.properties));
  ASSERT_EQ(ca_sn, value_of("dds.ca.sn", t.properties));

  ASSERT_EQ(std::string("RSA-2048"), value_of("dds.cert.algo", t.properties));
  ASSERT_EQ(std::string("RSA-2048"), value_of("dds.ca.algo", t.properties));
}

#if 0
TEST_F(AuthenticationTest, ValidateRemoteIdentity_Success)
{
  AuthenticationBuiltInImpl auth;
  IdentityHandle h;
  GUID_t adjusted;
  ValidationResult_t r = auth.validate_local_identity(h, adjusted, domain_id, domain_participant_qos, guid, ex);


}
#endif


#if 0
TEST_F(AuthenticationTest, NoLocalIdentity)
{
  // This will just do some simple testing of calling various
  // functions of the API with an invalid local identity handle
  AuthenticationBuiltInImpl test_class;
  CallFunctionsWithInvalidLocalHandle(test_class, 0);
}

TEST_F(AuthenticationTest, WrongLocalIdentity)
{
  // This will just do some simple testing of calling various
  // functions of the API with an invalid local identity handle
  // In this test, there will be an identity registered, but the
  // wrong handle will be used
  AuthenticationBuiltInImpl test_class;
  IdentityHandle local_handle = ValidateLocalParticipant(test_class);
  CallFunctionsWithInvalidLocalHandle(test_class, local_handle + 1);
}

TEST_F(AuthenticationTest, TestValidateRemoteIdentity)
{
  // This will just do some simple testing of calling various
  // functions of the API with an invalid local identity handle
  AuthenticationBuiltInImpl test_class;
  SecurityException ex;
  IdentityHandle handle = ValidateLocalParticipant(test_class);

  // Get tokens
  IdentityToken token;
  EXPECT_TRUE(test_class.get_identity_token(token, handle, ex));
  IdentityStatusToken status_token;
  EXPECT_TRUE(test_class.get_identity_token(status_token, handle, ex));

  PermissionsCredentialToken cred_token;
  PermissionsToken perm_token;
  EXPECT_TRUE(test_class.set_permissions_credential_and_token(handle, cred_token, perm_token, ex));

  // Can't validate a remote paticipant without validating local first
  IdentityHandle remote_handle_out = 0;
  IdentityToken remote_token;
  AuthRequestMessageToken local_auth_request_token;
  AuthRequestMessageToken remote_auth_request_token;
  GUID_t remote_guid;

  // The remote token needs to have a valid ID
  remote_token.class_id = "DDS:Auth:PKI-DH:1.0";

  ValidationResult_t validate_result =
    test_class.validate_remote_identity(
      remote_handle_out,
      local_auth_request_token,
      remote_auth_request_token,
      handle,
      remote_token,
      remote_guid,
      ex);
  EXPECT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST, validate_result);
}

#endif

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}