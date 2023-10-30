import {IconSearch}              from "@tabler/icons-react";
import {WithTranslationStore}    from "@use-pico2/i18n";
import {
    IQueryStore,
    type QuerySchema
}                                from "@use-pico2/query";
import {type WithIdentitySchema} from "@use-pico2/schema";
import {
    type IWithSourceQuery,
    useCount
}                                from "@use-pico2/source";
import {
    Container,
    Result,
    Status,
    WithIcon
}                                from "@use-pico2/ui";
import {
    type FC,
    useCallback
}                                from "react";

export namespace TableCountResult {
    export interface Props<
        TQuerySchema extends QuerySchema<any, any>,
        TSchema extends WithIdentitySchema,
    > {
        withQueryStore: IQueryStore<TQuerySchema>;
        withSourceQuery: IWithSourceQuery<TQuerySchema, TSchema>;
        Empty?: FC;
    }
}

export const TableCountResult = <
    TQuerySchema extends QuerySchema<any, any>,
    TSchema extends WithIdentitySchema,
>(
    {
        withQueryStore,
        withSourceQuery,
        Empty,
    }: TableCountResult.Props<TQuerySchema, TSchema>
) => {
    const {namespace} = WithTranslationStore.use(({namespace}) => ({namespace}));
    const countResult = useCount({
        store: withQueryStore,
        withSourceQuery,
    });

    const Empty$ = useCallback(() => <Status
        title={"empty.title"}
        message={"empty.message"}
    />, []);
    const WithEmpty = Empty || Empty$;

    return <>
        {countResult.data && !countResult.data.count && countResult.data.where > 0 && <Container size={"md"}>
            <Result
                icon={<WithIcon
                    size={"xl"}
                    icon={<IconSearch size={256}/>}
                />}
                withTranslation={{
                    namespace,
                    label: "filtered",
                }}
            />
        </Container>}
        {countResult.data && !countResult.data.where && <WithEmpty/>}
    </>;
};
