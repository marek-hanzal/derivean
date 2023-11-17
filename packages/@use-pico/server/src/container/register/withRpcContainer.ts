import {type IContainer} from "../../api/IContainer";
import {RpcService}      from "../../service/RpcService";
import {withRpcService}  from "../service/withRpcService";

export const withRpcContainer: IContainer.Register = container => {
    withRpcService.bind(container, RpcService);
};
