import {
    KingdomQueryProvider,
    KingdomTable
}                 from "@derivean/kingdom";
import {GameIcon} from "@derivean/ui";
import {Page}     from "@use-pico/client";
import {t}        from "@use-pico/translator";

export default function Index() {
    return <Page
        icon={<GameIcon/>}
        text={{
            header: t`Game overview`,
        }}
    >
        <div className={"container mx-auto"}>
            <KingdomQueryProvider
                values={{}}
            >
                <KingdomTable
                    compact
                />
            </KingdomQueryProvider>
        </div>
    </Page>;
}
