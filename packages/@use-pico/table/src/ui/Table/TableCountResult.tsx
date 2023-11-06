import {IconSearch}              from "@tabler/icons-react";
import {tx}                      from "@use-pico/i18n";
import {
    IQueryStore,
    type QuerySchema
}                                from "@use-pico/query";
import {type WithIdentitySchema} from "@use-pico/schema";
import {
    type IWithSourceQuery,
    useCount
}                                from "@use-pico/source";
import {
    Container,
    Loader,
    Result,
    Status,
    WithIcon
}                                from "@use-pico/ui";
import {
    type FC,
    useCallback
}                                from "react";

export namespace TableCountResult {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
        TSchema extends WithIdentitySchema,
    > {
        label?: {
            filtered?: Result.Props["label"];
            loading?: Result.Props["label"];
        };
        withQueryStore: IQueryStore.Store<TQuerySchema>;
        withSourceQuery: IWithSourceQuery<TQuerySchema, TSchema>;
        Empty?: FC;
    }
}

export const TableCountResult = <
    TQuerySchema extends QuerySchema<any, any>,
    TSchema extends WithIdentitySchema,
>(
    {
        label,
        withQueryStore,
        withSourceQuery,
        Empty,
    }: TableCountResult.Props<TQuerySchema, TSchema>
) => {
    const countResult = useCount({
        store: withQueryStore,
        withSourceQuery,
    });

    const Empty$ = useCallback(() => <Status
        label={{
            title:   tx()`Table is empty`,
            message: tx()`There is currently nothing to see`,
        }}
    />, []);
    const WithEmpty = Empty || Empty$;

    return <>
        {countResult.data && !countResult.data.count && countResult.data.where > 0 && <Container size={"md"}>
            <Result
                icon={<WithIcon
                    size={"xl"}
                    icon={<IconSearch size={256}/>}
                />}
                label={label?.filtered ?? {
                    title:    tx()`Nothing found by current filter`,
                    subtitle: tx()`Currently set filter is too strict, so there is nothing to show`,
                }}
            />
        </Container>}
        {countResult.data && !countResult.data.where && <WithEmpty/>}
        {countResult.isLoading && <Container size={"md"}>
            <Result
                icon={<WithIcon
                    size={"xl"}
                    icon={<Loader/>}
                />}
                label={label?.loading ?? {
                    title:    tx()`Loading data`,
                    subtitle: tx()`We're preparing all the data for you (if any)...`,
                }}
            />
        </Container>}
    </>;
};
