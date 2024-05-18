#include <iostream>
#include <windows.h>
#include <aclapi.h>
#include <unistd.h>

// Convert Linux mode to Windows file attributes
DWORD mode_to_attributes(mode_t mode) {
    DWORD attributes = 0;

    if (S_ISDIR(mode)) {
        attributes |= FILE_ATTRIBUTE_DIRECTORY;
    } else {
        attributes |= FILE_ATTRIBUTE_NORMAL;
    }

    if (mode & S_IRUSR) attributes |= FILE_ATTRIBUTE_READONLY;
    if (mode & S_IWUSR) attributes &= ~FILE_ATTRIBUTE_READONLY;
    if (mode & S_IXUSR) attributes |= FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_READONLY;
/*  if (mode & S_IRGRP) attributes |= FILE_ATTRIBUTE_READONLY;
    if (mode & S_IWGRP) attributes &= ~FILE_ATTRIBUTE_READONLY;
    if (mode & S_IXGRP) attributes |= FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_READONLY;
    if (mode & S_IROTH) attributes |= FILE_ATTRIBUTE_READONLY;
    if (mode & S_IWOTH) attributes &= ~FILE_ATTRIBUTE_READONLY;
    if (mode & S_IXOTH) attributes |= FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_READONLY;
*/
    return attributes;
}

// Convert Linux mode to Windows security descriptor
BOOL mode_to_security_descriptor(const char *path, mode_t /*mode*/) {
    PSID pOwnerSID = NULL;
    PACL pACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESS ea;
    DWORD dwRes;
    BOOL bSuccess = FALSE;
    PTOKEN_USER pTokenUser = NULL;

    // Get the current user's SID
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        goto Cleanup;
    }

    DWORD dwBufferSize = 0;
    GetTokenInformation(hToken, TokenUser, NULL, 0, &dwBufferSize);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        goto Cleanup;
    }

    pTokenUser = (PTOKEN_USER)LocalAlloc(LPTR, dwBufferSize);
    if (pTokenUser == NULL) {
        goto Cleanup;
    }

    if (!GetTokenInformation(hToken, TokenUser, pTokenUser, dwBufferSize, &dwBufferSize)) {
        goto Cleanup;
    }

    pOwnerSID = pTokenUser->User.Sid;

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));

    // Set permissions for the file owner.
//    ea.grfAccessPermissions = mode_to_attributes(mode);
    ea.grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
    ea.grfAccessMode = SET_ACCESS;
    ea.grfInheritance = NO_INHERITANCE;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
    ea.Trustee.ptstrName = (LPTSTR)pOwnerSID;

    // Create a new ACL that contains the new ACEs.
    dwRes = SetEntriesInAcl(1, &ea, NULL, &pACL);
    if (ERROR_SUCCESS != dwRes) {
        goto Cleanup;
    }

    // Initialize a security descriptor.
    pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (NULL == pSD) {
        goto Cleanup;
    }

    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) {
        goto Cleanup;
    }

    // Add the ACL to the security descriptor.
    if (!SetSecurityDescriptorDacl(pSD, TRUE, pACL, FALSE)) {
        goto Cleanup;
    }

    // Set the security descriptor for the file.
    if (ERROR_SUCCESS != SetNamedSecurityInfoA((LPSTR)path,
                                                SE_FILE_OBJECT,
                                                DACL_SECURITY_INFORMATION,
                                                pOwnerSID,
                                                NULL,
                                                pACL,
                                                NULL)) {
        goto Cleanup;
    }

    bSuccess = TRUE;

Cleanup:

    if (pACL) {
        LocalFree(pACL);
    }
    if (pSD) {
        LocalFree(pSD);
    }
    if (pTokenUser) {
        LocalFree(pTokenUser);
    }
    if (hToken) {
        CloseHandle(hToken);
    }

    return bSuccess;
}

// chmod function for Windows
// Uses:
// current process user as owner SID
// Default Users group as group SID
// Default Everyone group for other
int win_chmod(const char *filename, mode_t mode) {
    PACL pOldDACL = nullptr, pNewDACL = nullptr;
    PSECURITY_DESCRIPTOR pSD = nullptr;
    EXPLICIT_ACCESS explicitAccess[5];
    int nResult = -1;

    // Get a pointer to the existing DACL
    if (GetNamedSecurityInfo(filename, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &pOldDACL, nullptr, &pSD) != ERROR_SUCCESS) {
        return -1;
    } else {
        // Get Administrators group SID
        PSID pSidAdmin = nullptr;
        SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
        
        if (AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pSidAdmin)) {

            // Get permissions for Users group
            PSID pSidUsers = nullptr;
            SID_IDENTIFIER_AUTHORITY SIDAutGroup = SECURITY_NT_AUTHORITY;
            if (AllocateAndInitializeSid(&SIDAutGroup, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS, 0, 0, 0, 0, 0, 0, &pSidUsers)) {

                // Get permissions for Everyone group
                PSID pSidEveryone = nullptr;
                
                SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
                if (AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pSidEveryone)) {

                    // Initialize EXPLICIT_ACCESS structures
                    ZeroMemory(&explicitAccess, sizeof(explicitAccess));

                    // Set permissions for SYSTEM
                    explicitAccess[0].grfAccessPermissions = GENERIC_ALL;
                    explicitAccess[0].grfAccessMode = GRANT_ACCESS;
                    explicitAccess[0].grfInheritance = NO_INHERITANCE;
                    explicitAccess[0].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
                    explicitAccess[0].Trustee.ptstrName = const_cast<LPSTR>("SYSTEM");

                    // Set permissions for Administrators
                    explicitAccess[1].grfAccessPermissions = GENERIC_ALL;
                    explicitAccess[1].grfAccessMode = GRANT_ACCESS;
                    explicitAccess[1].grfInheritance = NO_INHERITANCE;
                    explicitAccess[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
                    explicitAccess[1].Trustee.ptstrName = (LPTSTR)pSidAdmin;

                    // Set permissions for current user
                    explicitAccess[2].grfAccessPermissions |= (((mode & S_IRUSR) == S_IRUSR) ? GENERIC_READ : 0);
                    explicitAccess[2].grfAccessPermissions |= (((mode & S_IWUSR) == S_IWUSR) ? GENERIC_WRITE : 0);
                    explicitAccess[2].grfAccessPermissions |= (((mode & S_IXUSR) == S_IXUSR) ? GENERIC_EXECUTE : 0);
                    explicitAccess[2].grfAccessMode = GRANT_ACCESS;
                    explicitAccess[2].grfInheritance = NO_INHERITANCE;
                    explicitAccess[2].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
                    explicitAccess[2].Trustee.ptstrName = const_cast<LPSTR>("CURRENT_USER");

                    // Set permissions for Users
                    explicitAccess[3].grfAccessPermissions |= (((mode & S_IRGRP) == S_IRGRP) ? GENERIC_READ : 0);
                    explicitAccess[3].grfAccessPermissions |= (((mode & S_IWGRP) == S_IWGRP) ? GENERIC_WRITE : 0);
                    explicitAccess[3].grfAccessPermissions |= (((mode & S_IXGRP) == S_IXGRP) ? GENERIC_EXECUTE : 0);
                    explicitAccess[3].grfAccessMode = GRANT_ACCESS;
                    explicitAccess[3].grfInheritance = NO_INHERITANCE;
                    explicitAccess[3].Trustee.TrusteeForm = TRUSTEE_IS_SID;
                    explicitAccess[3].Trustee.ptstrName = (LPTSTR)pSidUsers;

                    // Set permissions for Everyone
                    explicitAccess[4].grfAccessPermissions |= (((mode & S_IROTH) == S_IROTH) ? GENERIC_READ : 0);
                    explicitAccess[4].grfAccessPermissions |= (((mode & S_IWOTH) == S_IWOTH) ? GENERIC_WRITE : 0);
                    explicitAccess[4].grfAccessPermissions |= (((mode & S_IXOTH) == S_IXOTH) ? GENERIC_EXECUTE : 0);
                    explicitAccess[4].grfAccessMode = GRANT_ACCESS;
                    explicitAccess[4].grfInheritance = NO_INHERITANCE;
                    explicitAccess[4].Trustee.TrusteeForm = TRUSTEE_IS_SID;
                    explicitAccess[4].Trustee.ptstrName = (LPTSTR)pSidEveryone;


                    // Create a new ACL that merges the new ACE into the existing DACL
                    if (SetEntriesInAcl(5, explicitAccess, NULL, &pNewDACL) == ERROR_SUCCESS) {
                        // Set the new DACL to the new security descriptor
                        if (SetSecurityDescriptorDacl(pSD, TRUE, pNewDACL, FALSE) == ERROR_SUCCESS) {
                            // Set the new security descriptor for the file
                            if (SetNamedSecurityInfo(const_cast<char*>(filename), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, NULL, NULL, pNewDACL, NULL) == ERROR_SUCCESS) {
                                nResult = 0;
                            }
                        }
                        if (pNewDACL)
                            LocalFree(pNewDACL);
                    }
                    FreeSid(pSidEveryone);
                }

                FreeSid(pSidUsers);
            }

            FreeSid(pSidAdmin);
        }

        if (pSD)
            LocalFree(pSD);
    }

    return nResult;
}

#undef stat
int win_stat(const char *filename, struct stat *buffer) {

    if (!filename || !buffer)
        return -1;
    
    stat(filename, buffer);
    // Get the security descriptor for the file
    PSECURITY_DESCRIPTOR pSecurityDescriptor = nullptr;
    if (ERROR_SUCCESS != GetNamedSecurityInfo((LPCSTR)filename, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, nullptr, nullptr, &pSecurityDescriptor)) {
        return 0;
    }

    // Get the DACL from the security descriptor
    PACL pDACL;
    BOOL bDACLPresent, bDACLDefaulted;
    if (!GetSecurityDescriptorDacl(pSecurityDescriptor, &bDACLPresent, &pDACL, &bDACLDefaulted)) {
        LocalFree(pSecurityDescriptor);
        return 0;
    }

    // Check if the DACL is present and not defaulted
    if (!bDACLPresent || bDACLDefaulted || !pDACL) {
        LocalFree(pSecurityDescriptor);
        return 0;
    }

    // Linux Mask
    mode_t mask = 0;
    ////////////////////////////////
    // Get permissions for Current User

    // Get the current user's SID
    HANDLE hToken = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        DWORD dwBufferSize;
        PTOKEN_USER pTokenUser = nullptr;
        GetTokenInformation(hToken, TokenUser, nullptr, 0, &dwBufferSize);
        pTokenUser = (PTOKEN_USER)LocalAlloc(LPTR, dwBufferSize);

        if (pTokenUser) {
            if (GetTokenInformation(hToken, TokenUser, pTokenUser, dwBufferSize, &dwBufferSize)) {
                TRUSTEE Trustee = {0};
                BuildTrusteeWithSid(&Trustee, pTokenUser->User.Sid);
                ACCESS_MASK accessMask;
                DWORD dwRetVal = ::GetEffectiveRightsFromAcl( pDACL,
                                    &Trustee,
                                &accessMask);
                if (dwRetVal == ERROR_SUCCESS) {
                    mask |= (((accessMask & FILE_GENERIC_READ) == FILE_GENERIC_READ) ? S_IRUSR : 0);
                    mask |= (((accessMask & FILE_GENERIC_WRITE) == FILE_GENERIC_WRITE) ? S_IWUSR : 0);
                    mask |= (((accessMask & FILE_GENERIC_EXECUTE) == FILE_GENERIC_EXECUTE) ? S_IXUSR : 0);
                }
            }
            LocalFree(pTokenUser);
        }
        CloseHandle(hToken);
    }

    ////////////////////////////////
    // Get permissions for Group
    PSID pSid = nullptr;
    SID_IDENTIFIER_AUTHORITY SIDAutGroup = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&SIDAutGroup, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_USERS, 0, 0, 0, 0, 0, 0, &pSid)) {

        TRUSTEE GroupTrustee;
        BuildTrusteeWithSid(&GroupTrustee, pSid);
        ACCESS_MASK GroupAccessMask;
        if (GetEffectiveRightsFromAcl(pDACL, &GroupTrustee, &GroupAccessMask) == ERROR_SUCCESS) {
            mask |= (((GroupAccessMask & FILE_GENERIC_READ) == FILE_GENERIC_READ) ? S_IRGRP : 0);
            mask |= (((GroupAccessMask & FILE_GENERIC_WRITE) == FILE_GENERIC_WRITE) ? S_IWGRP : 0);
            mask |= (((GroupAccessMask & FILE_GENERIC_EXECUTE) == FILE_GENERIC_EXECUTE) ? S_IXGRP : 0);
        }

        FreeSid(pSid);
    }

    ////////////////////////////////
    // Get permissions for Other
    pSid = nullptr;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    if (AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID,
                                  0, 0, 0, 0, 0, 0, 0, &pSid)) {
        
        TRUSTEE EveryoneTrustee;
        BuildTrusteeWithSid(&EveryoneTrustee, pSid);

        ACCESS_MASK EveryoneAccessMask;
        if (GetEffectiveRightsFromAcl(pDACL, &EveryoneTrustee, &EveryoneAccessMask) == ERROR_SUCCESS) {
            mask |= (((EveryoneAccessMask & FILE_GENERIC_READ) == FILE_GENERIC_READ) ? S_IROTH : 0);
            mask |= (((EveryoneAccessMask & FILE_GENERIC_WRITE) == FILE_GENERIC_WRITE) ? S_IWOTH : 0);
            mask |= (((EveryoneAccessMask & FILE_GENERIC_EXECUTE) == GENERIC_EXECUTE) ? S_IXOTH : 0);
        }

        FreeSid(pSid);
    }

    LocalFree(pSecurityDescriptor);
    buffer->st_mode = mask;
    return 0;
}
