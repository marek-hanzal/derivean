import {type IContainer}   from "@use-pico/container";
import {type IUserSession} from "../api/IUserSession";
import {withUserSession}   from "../api/withUserSession";

export const useUserSession = (container: IContainer.Type) => container.resolve<IUserSession>(withUserSession);
