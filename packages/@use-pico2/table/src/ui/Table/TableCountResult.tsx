import {IconSearch}              from "@tabler/icons-react";
import {WithTranslationStore}    from "@use-pico2/i18n";
import {
    type FilterSchema,
    type IWithSourceQuery,
    type OrderBySchema,
    type QuerySchema
}                                from "@use-pico2/query";
import {type WithIdentitySchema} from "@use-pico2/schema";
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
        TFilterSchema extends FilterSchema,
        TOrderBySchema extends OrderBySchema,
        TQuerySchema extends QuerySchema<TFilterSchema, TOrderBySchema>,
        TSchema extends WithIdentitySchema,
    > {
        withSourceQuery: IWithSourceQuery<TFilterSchema, TOrderBySchema, TQuerySchema, TSchema>;
        Empty?: FC;
    }
}

export const TableCountResult = <
    TFilterSchema extends FilterSchema,
    TOrderBySchema extends OrderBySchema,
    TQuerySchema extends QuerySchema<TFilterSchema, TOrderBySchema>,
    TSchema extends WithIdentitySchema,
>(
    {
        withSourceQuery,
        Empty,
    }: TableCountResult.Props<TFilterSchema, TOrderBySchema, TQuerySchema, TSchema>
) => {
    const {namespace} = WithTranslationStore.use(({namespace}) => ({namespace}));
    const countResult = withSourceQuery.useCount();

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
