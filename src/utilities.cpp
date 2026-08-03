#include "utilities.h"

std::string strengthLevelToString(StrengthLevel level) {
    switch (level) {
        case StrengthLevel::WEAK: return "WEAK";
        case StrengthLevel::FAIR: return "FAIR";
        case StrengthLevel::GOOD: return "GOOD";
        case StrengthLevel::STRONG: return "STRONG";
        case StrengthLevel::VERY_STRONG: return "VERY STRONG";
    }
    return "UNKNOWN";
}

std::string auditActionToString(AuditAction action) {
    switch (action) {
        case AuditAction::LOGIN: return "login";
        case AuditAction::LOGOUT: return "logout";
        case AuditAction::ADD_ENTRY: return "add_entry";
        case AuditAction::UPDATE_ENTRY: return "update_entry";
        case AuditAction::DELETE_ENTRY: return "delete_entry";
        case AuditAction::CHANGE_MASTER_PASSWORD: return "change_master_password";
        case AuditAction::VAULT_CREATED: return "vault_created";
    }
    return "unknown";
}
