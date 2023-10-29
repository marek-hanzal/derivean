import {type IRegistrationService} from "@use-pico2/auth";
import {withService}               from "@use-pico2/container";

export const withRegistrationService = withService<IRegistrationService>("@use-pico2/auth/RegistrationService");
