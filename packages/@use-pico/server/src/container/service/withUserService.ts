import {type IUserService} from "../../api/IUserService";
import {withService}       from "../../service/withService";

export const withUserService = withService<IUserService>("@use-pico/server/UserService");
