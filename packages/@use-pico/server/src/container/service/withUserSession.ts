import {type IUserSession} from "../../api/IUserSession";
import {withService}       from "../../service/withService";

export const withUserSession = withService<IUserSession>("@use-pico/server/UserSession");
