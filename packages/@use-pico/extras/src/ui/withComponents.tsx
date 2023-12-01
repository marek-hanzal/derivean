import {
    Form,
    type IQueryStore,
    Table
}                       from "@use-pico/client";
import {type Rpc}       from "../api/Rpc";
import {type Schema}    from "../api/Schema";
import {withCollection} from "./withCollection";
import {withFetch}      from "./withFetch";

export namespace withComponents {
    export interface Props<
        TRpc extends Rpc<Schema<any, any, any, any>>,
    > {
        rpc: TRpc;
        queryStore: IQueryStore.Store<TRpc["schema"]["query"]>;
    }
}

/**
 * This is a factory method for a lot of common components which has to be created manually otherwise.
 */
export const withComponents = <
    TRpc extends Rpc<Schema<any, any, any, any>>,
>(
    {
        rpc,
        queryStore,
    }: withComponents.Props<TRpc>,
) => {
    return {
        Fetch:      withFetch<TRpc["schema"]["query"], TRpc["schema"]["entity"]>({withQuery: rpc.query}),
        Collection: withCollection<TRpc["schema"]["query"], TRpc["schema"]["entity"]>({withSourceQuery: rpc.query}),
        Form:  {
            Mutation: (
                          props: Omit<
                              Form.Props<TRpc["schema"]>,
                              "schema"
                          >,
                      ) => <Form<TRpc["schema"]>
                // withMutation={rpc.mutation}
                schema={rpc.schema}
                // values={entity}
                {...props}
            />,
        },
        Table: <
                   TColumns extends string,
               >(
            props: Omit<
                Table.Props<
                    TColumns,
                    TRpc["schema"]
                >,
                "withSourceQuery" | "withQueryStore"
            >
        ) => <Table
            withSourceQuery={rpc.query}
            withQueryStore={queryStore}
            {...props}
        />,
    };
};
