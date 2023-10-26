import {type IContainer}        from "@use-pico/container";
import {type IUserTokenService} from "../api/IUserTokenService";
import {withUserTokenService}   from "../api/withUserTokenService";

export const useUserTokenService = (container: IContainer.Type) => container.resolve<IUserTokenService>(withUserTokenService);
