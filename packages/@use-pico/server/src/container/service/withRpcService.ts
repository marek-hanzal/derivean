import {type IRpcService} from "../../api/IRpcService";
import {withService}      from "../../service/withService";

export const withRpcService = withService<IRpcService>("@use-pico/server/RpcService");
