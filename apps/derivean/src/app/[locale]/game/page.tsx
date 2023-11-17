import {GameIcon} from "@derivean/ui";
import {Page}     from "@use-pico/client";
import {t}        from "@use-pico/translator";

export default function Index() {
    return <Page
        icon={<GameIcon/>}
        text={{
            header: t()`Game overview`,
        }}
    >
        {/*<Container>*/}
        {/*    <StoreProvider*/}
        {/*        store={KingdomQueryStore}*/}
        {/*        values={{}}*/}
        {/*    >*/}
        {/*        <KingdomTable*/}
        {/*            compact*/}
        {/*        />*/}
        {/*    </StoreProvider>*/}
        {/*</Container>*/}
    </Page>;
}
