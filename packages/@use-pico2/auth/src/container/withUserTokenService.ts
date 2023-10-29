import {type IUserTokenService} from "@use-pico2/auth";
import {withService}            from "@use-pico2/container";

export const withUserTokenService = withService<IUserTokenService>("@use-pico2/auth/UserTokenService");
