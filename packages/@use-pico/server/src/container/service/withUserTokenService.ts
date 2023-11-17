import {type IUserTokenService} from "../../api/IUserTokenService";
import {withService}            from "../../service/withService";

export const withUserTokenService = withService<IUserTokenService>("@use-pico/server/UserTokenService");
