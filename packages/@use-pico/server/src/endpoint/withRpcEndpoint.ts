import {
    type NextRequest,
    NextResponse
}                        from "next/server";
import {type IContainer} from "../api/IContainer";
import {withRpcService}  from "../container/service/withRpcService";

export namespace withRpcEndpoint {
    export interface Props {
        /**
         * Global container
         */
        container: IContainer.Type;
        /**
         * Context container factory (for example, for services needing to access a user session)
         */
        context?: IContainer.Register;
    }
}

export const withRpcEndpoint = (
    {
        container,
        context,
    }: withRpcEndpoint.Props,
) => {
    const rpcService = withRpcService.use(container);
    return async (request: NextRequest, response: NextResponse) => rpcService.handle({
        context,
        request,
        response,
    });
};
