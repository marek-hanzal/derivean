import {type IUserSession} from "@use-pico2/auth";
import {withService}       from "@use-pico2/container";

export const withUserSession = withService<IUserSession>("@use-pico2/auth/UserSession");
