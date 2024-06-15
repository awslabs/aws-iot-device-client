#include <iostream>
#include <aclapi.h>
#include <unistd.h>
#include <signal.h>
#include <winioctl.h>


// Get "as-is" access (not "effective") from the object
ACCESS_MASK GetAccessRights(PACL pAcl, TRUSTEE* pTrustee) {
    ACCESS_MASK accessMask = 0;

    // Iterate through ACEs in the ACL
    for (DWORD i = 0; i < pAcl->AceCount; i++) {
        PACE_HEADER pAceHeader = NULL;
        if (!GetAce(pAcl, i, (LPVOID*)&pAceHeader)) {
            return 0;
        }

        // Check if the ACE applies to the specified trustee
        if (EqualSid(pTrustee->ptstrName, &((PACCESS_ALLOWED_ACE)pAceHeader)->SidStart)) {
            // Extract access rights from the ACE
            accessMask |= ((PACCESS_ALLOWED_ACE)pAceHeader)->Mask;
        }
    }

    return accessMask;
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
                    explicitAccess[2].grfAccessPermissions |= (((mode & S_IRUSR) == S_IRUSR) ? FILE_GENERIC_READ : 0);
                    explicitAccess[2].grfAccessPermissions |= (((mode & S_IWUSR) == S_IWUSR) ? FILE_GENERIC_WRITE : 0);
                    explicitAccess[2].grfAccessPermissions |= (((mode & S_IXUSR) == S_IXUSR) ? FILE_GENERIC_EXECUTE : 0);
                    explicitAccess[2].grfAccessMode = GRANT_ACCESS;
                    explicitAccess[2].grfInheritance = NO_INHERITANCE;
                    explicitAccess[2].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
                    explicitAccess[2].Trustee.ptstrName = const_cast<LPSTR>("CURRENT_USER");

                    // Set permissions for Users
                    explicitAccess[3].grfAccessPermissions |= (((mode & S_IRGRP) == S_IRGRP) ? FILE_GENERIC_READ : 0);
                    explicitAccess[3].grfAccessPermissions |= (((mode & S_IWGRP) == S_IWGRP) ? FILE_GENERIC_WRITE : 0);
                    explicitAccess[3].grfAccessPermissions |= (((mode & S_IXGRP) == S_IXGRP) ? FILE_GENERIC_EXECUTE : 0);
                    explicitAccess[3].grfAccessMode = GRANT_ACCESS;
                    explicitAccess[3].grfInheritance = NO_INHERITANCE;
                    explicitAccess[3].Trustee.TrusteeForm = TRUSTEE_IS_SID;
                    explicitAccess[3].Trustee.ptstrName = (LPTSTR)pSidUsers;

                    // Set permissions for Everyone
                    explicitAccess[4].grfAccessPermissions |= (((mode & S_IROTH) == S_IROTH) ? FILE_GENERIC_READ : 0);
                    explicitAccess[4].grfAccessPermissions |= (((mode & S_IWOTH) == S_IWOTH) ? FILE_GENERIC_WRITE : 0);
                    explicitAccess[4].grfAccessPermissions |= (((mode & S_IXOTH) == S_IXOTH) ? FILE_GENERIC_EXECUTE : 0);
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


    ZeroMemory(buffer, sizeof(struct stat));
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
                ACCESS_MASK tmpMask;
                tmpMask = GetAccessRights(pDACL, &Trustee);
                DWORD dwRetVal = ::GetEffectiveRightsFromAcl( pDACL,
                                    &Trustee,
                                &accessMask);
                if (dwRetVal == ERROR_SUCCESS) {
                    mask |= (((tmpMask & FILE_GENERIC_READ) == FILE_GENERIC_READ) ? S_IRUSR : 0);
                    mask |= (((tmpMask & FILE_GENERIC_WRITE) == FILE_GENERIC_WRITE) ? S_IWUSR : 0);
                    mask |= (((tmpMask & FILE_GENERIC_EXECUTE) == FILE_GENERIC_EXECUTE) ? S_IXUSR : 0);
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
        ACCESS_MASK GroupAccessMask, tmpMask;
        tmpMask = GetAccessRights(pDACL, &GroupTrustee);
        if (GetEffectiveRightsFromAcl(pDACL, &GroupTrustee, &GroupAccessMask) == ERROR_SUCCESS) {
            mask |= (((tmpMask & FILE_GENERIC_READ) == FILE_GENERIC_READ) ? S_IRGRP : 0);
            mask |= (((tmpMask & FILE_GENERIC_WRITE) == FILE_GENERIC_WRITE) ? S_IWGRP : 0);
            mask |= (((tmpMask & FILE_GENERIC_EXECUTE) == FILE_GENERIC_EXECUTE) ? S_IXGRP : 0);
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
        ACCESS_MASK tmpMask;
        tmpMask = GetAccessRights(pDACL, &EveryoneTrustee);
        if (GetEffectiveRightsFromAcl(pDACL, &EveryoneTrustee, &EveryoneAccessMask) == ERROR_SUCCESS) {
            mask |= (((tmpMask & FILE_GENERIC_READ) == FILE_GENERIC_READ) ? S_IROTH : 0);
            mask |= (((tmpMask & FILE_GENERIC_WRITE) == FILE_GENERIC_WRITE) ? S_IWOTH : 0);
            mask |= (((tmpMask & FILE_GENERIC_EXECUTE) == FILE_GENERIC_EXECUTE) ? S_IXOTH : 0);
        }

        FreeSid(pSid);
    }

    LocalFree(pSecurityDescriptor);
    
    stat(filename, buffer);
    if (buffer->st_mode & S_IFDIR)
        mask |= S_IFDIR;
    
    buffer->st_mode = mask;
    return 0;
}

// wrapper for POSIX open method
// uses chmod inside to set file permissions as defined in _pMode
int win_open(
    _In_z_ char const* const _FileName,
    _In_   int         const _OFlag,
    _In_   int         const _PMode) {

    if (-1 == _open(_FileName, _OFlag, _O_RDWR))
        return -1;
    
    return win_chmod(_FileName, (mode_t)_PMode);
}

int win_mkdir(const char *pathname, mode_t mode) {
    if (_mkdir(pathname) == 0) {        
        DWORD fileAttr = GetFileAttributes(pathname);
        if (fileAttr == INVALID_FILE_ATTRIBUTES) {
        // GetLastError can be used here to get more error details
            return -1;
        }
        else {
            return win_chmod(pathname, mode);       
        }        
    } else {
        return -1; // Error
    }
}

int kill(pid_t pid, int sig) {
    DWORD dwCtrlEvent;

    switch (sig) {
        case SIGINT:
            dwCtrlEvent = CTRL_C_EVENT;
            break;
        case SIGTERM:
            dwCtrlEvent = CTRL_BREAK_EVENT;
            break;
        default:
            // Unsupported signal
            return -1;
    }

    if (pid == 0) {
        // Send the signal to all processes in the current group
        if (!GenerateConsoleCtrlEvent(dwCtrlEvent, 0)) {
            return -1;
        }
    } else {
        // Send the signal to the specified process
        if (!GenerateConsoleCtrlEvent(dwCtrlEvent, pid)) {
            return -1;
        }
    }

    return 0;
}

uid_t getuid() {
    HANDLE hToken;
    DWORD dwLengthNeeded;
    PTOKEN_USER pTokenUser;
    TCHAR lpUserName[256];
    DWORD dwUserNameSize = sizeof(lpUserName);
    char domain[256];
    DWORD domainSize = sizeof(domain);
    SID_NAME_USE use;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        return -1;
    }

    if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwLengthNeeded) &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        CloseHandle(hToken);
        return -1;
    }

    pTokenUser = (PTOKEN_USER)GlobalAlloc(GPTR, dwLengthNeeded);
    if (!pTokenUser) {
        CloseHandle(hToken);
        return -1;
    }

    if (!GetTokenInformation(hToken, TokenUser, pTokenUser, dwLengthNeeded, &dwLengthNeeded)) {
        GlobalFree(pTokenUser);
        CloseHandle(hToken);
        return -1;
    }

    if (!LookupAccountSid(NULL, pTokenUser->User.Sid, lpUserName, &dwUserNameSize, domain, &domainSize, &use)) {
        GlobalFree(pTokenUser);
        CloseHandle(hToken);
        return -1;
    }

    // Convert SID to string representation
    LPTSTR stringSid;
    if (!ConvertSidToStringSid(pTokenUser->User.Sid, &stringSid)) {
        GlobalFree(pTokenUser);
        CloseHandle(hToken);
        return -1;
    }

    // Parse the string SID to extract the user ID
    uid_t uid = atoi(stringSid);
    
    // Free resources
    GlobalFree(pTokenUser);
    LocalFree(stringSid);
    CloseHandle(hToken);

    return uid;
}

long pathconf(const char *path, int name) {
    if (name != _PC_PATH_MAX) {
        return -1; // Unsupported parameter
    }

    char volume[MAX_PATH];
    DWORD max_path_length;
    
    if (!GetVolumeInformation(path, NULL, 0, NULL, &max_path_length, NULL, volume, MAX_PATH)) {
        return -1;
    }
    
    return max_path_length;
}

int setenv(const char *name, const char *value, int overwrite) {
    // If overwrite is 0 and the environment variable already exists, do nothing
    if (!overwrite && getenv(name) != NULL) {
        return 0;
    }
    
    return _putenv_s(name, value);
}

ssize_t readlink(const char */*path*/, char *buf, size_t len)
{
    DWORD size = GetModuleFileName(NULL, buf, (DWORD)len);
    if (size == 0) {
        return - 1;
    }
    else
        return size;
}
