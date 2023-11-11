import {Form}            from "@use-pico/form";
import {withFetch}       from "@use-pico/source-ui";
import {type WithEntity} from "@use-pico/types";
import {withDullRpc}     from "./withDullRpc";
import {withDullSchema}  from "./withDullSchema";

export namespace withDullUI {
    export interface Props<
        TRpc extends withDullRpc.Rpc<withDullSchema.Schema<any, any, any, any>>,
    > {
        rpc: TRpc;
    }
}

export const withDullUI = <
    TRpc extends withDullRpc.Rpc<withDullSchema.Schema<any, any, any, any>>,
>(
    {
        rpc,
    }: withDullUI.Props<TRpc>,
) => {
    return {
        Fetch:        withFetch({withQuery: rpc.query}),
        MutationForm: (
                          {
                              entity,
                              ...props
                          }: Omit<
                                 Form.Props<
                                     typeof rpc.mutation,
                                     TRpc["schema"]["shape"],
                                     TRpc["mutation"]["schema"]["request"],
                                     TRpc["mutation"]["schema"]["response"]
                                 >,
                                 "withMutation" | "schema"
                             >
                             & WithEntity.Schema.$<TRpc["schema"]["entity"]>,
                      ) => {
            return <Form
                withMutation={rpc.mutation}
                schema={rpc.schema.shape}
                values={entity}
                {...props}
            />;
        },
    };
};
